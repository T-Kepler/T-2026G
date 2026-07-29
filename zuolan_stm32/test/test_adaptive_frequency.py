"""128点分级FFT粗测＋128点过零精测回归，仅使用Python标准库。"""

import cmath
import math
from pathlib import Path


N = 128
RATES = (65_000_000.0, 65_000_000.0 / N, 65_000_000.0 / N / N)
ROOT = Path(__file__).resolve().parents[2]


def capture(freq, sample_rate, phase=0.37, square=False, ripple=False, size=N):
    samples = []
    for index in range(size):
        value = math.sin(2 * math.pi * freq * index / sample_rate + phase)
        if square:
            value = 1.0 if value >= 0.0 else -1.0
        raw = 2048 + 100 * value
        if ripple:
            raw += 10 * math.sin(2 * math.pi * 4_496_184 * index / sample_rate + 0.2)
        samples.append(round(raw))
    return samples


def coarse_peak(raw_samples, sample_rate):
    samples = [(value - 2048) * 10.0 / 2048 for value in raw_samples]
    mean = sum(samples) / N
    window = [0.5 * (1 - math.cos(2 * math.pi * i / (N - 1))) for i in range(N)]
    spectrum = []
    for bin_index in range(N // 2):
        total = sum(
            (samples[i] - mean) * window[i]
            * cmath.exp(-2j * math.pi * bin_index * i / N)
            for i in range(N)
        )
        spectrum.append(abs(total))

    peak = max(range(1, N // 2), key=spectrum.__getitem__)
    offset = 0.0
    if 1 < peak < N // 2 - 1:
        left, center, right = spectrum[peak - 1 : peak + 2]
        denominator = left - 2 * center + right
        if abs(denominator) >= 1e-10:
            offset = max(-0.5, min(0.5, 0.5 * (left - right) / denominator))
    return (peak + offset) * sample_rate / N, spectrum[peak] * 2 / sum(window)


def estimate(raw_samples, sample_rate):
    low, high = min(raw_samples), max(raw_samples)
    value_range = high - low
    if value_range < 12:
        return 0.0

    midpoint = low + value_range * 0.5
    arm_level = low + value_range * 0.4
    armed = raw_samples[0] <= arm_level
    crossings = []
    for index in range(1, len(raw_samples)):
        previous, current = raw_samples[index - 1 : index + 1]
        if current <= arm_level:
            armed = True
        if armed and previous < midpoint <= current and current > previous:
            crossings.append(index - 1 + (midpoint - previous) / (current - previous))
            armed = False

    if len(crossings) < 4:
        return 0.0
    frequency = sample_rate * (len(crossings) - 1) / (crossings[-1] - crossings[0])
    return frequency if frequency <= sample_rate * 0.25 else 0.0


def detect(freq, phase=0.37, square=False, ripple=False):
    candidates = [coarse_peak(capture(freq, rate, phase, square, ripple), rate)
                  for rate in RATES]
    max_magnitude = max(magnitude for _, magnitude in candidates)
    if max_magnitude < 0.02:
        return 0.0

    coarse = candidates[-1][0]
    for frequency, magnitude in candidates:
        if magnitude >= max_magnitude * 0.8:
            coarse = frequency
            break

    ratio = 8.0 if coarse > 60_000 else 24.0
    fine_rate = min(65_000_000.0, coarse * ratio)
    measured = estimate(capture(freq, fine_rate, phase + 0.41, square, ripple), fine_rate)
    if not measured:
        fine_rate *= 0.5
        measured = estimate(capture(freq, fine_rate, phase + 0.41, square, ripple), fine_rate)
    return measured


def choose_refined_frequency(zero_crossing, fft_peak):
    if fft_peak <= 0:
        return zero_crossing
    if zero_crossing <= 0 or abs(zero_crossing - fft_peak) > fft_peak * 0.01:
        return fft_peak
    return zero_crossing


if __name__ == "__main__":
    for frequency in (50.0, 5_000.0, 50_000.0, 500_000.0, 5_000_000.0):
        for phase in (0.1, 1.2, 2.7):
            for square in (False, True):
                measured = detect(frequency, phase, square)
                assert measured and abs(measured - frequency) / frequency < 0.01, (
                    frequency, phase, square, measured
                )

    measured = detect(5_000.0, phase=math.pi / 2, ripple=True)
    assert abs(measured - 5_000.0) / 5_000.0 < 0.01, measured
    failed_rate = 292_667.0
    assert estimate(capture(5_000.0, failed_rate), failed_rate) == 0.0
    retry = estimate(capture(5_000.0, failed_rate / 2), failed_rate / 2)
    assert abs(retry - 5_000.0) / 5_000.0 < 0.01
    for actual, coarse in ((5_000, 926.555), (50_000, 2_345.941),
                           (500_000, 164_799.297),
                           (5_000_000, 3_757_097.750)):
        ratio = 128 if coarse <= 100_000 else 64
        formal_rate = min(65_000_000, coarse * ratio)
        precise = estimate(capture(actual, formal_rate, size=1024), formal_rate)
        assert abs(precise - actual) / actual < 0.001, (actual, precise)
    assert choose_refined_frequency(1_056_319.875, 998_558.0) == 998_558.0
    assert choose_refined_frequency(500_028.906, 499_990.0) == 500_028.906
    print("128点自适应测频回归通过：50Hz～5MHz及5kHz高频纹波场景")
