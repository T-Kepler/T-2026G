"""FFT/THD最小回归：仅使用Python标准库。"""

import cmath
import math
from pathlib import Path


N = 1024
ROOT = Path(__file__).resolve().parents[2]


def formal_sample_rate(signal_freq):
    ratio = 128 if signal_freq <= 100_000 else 64
    target = signal_freq * ratio
    if target <= 65_000_000:
        return target
    cycles = math.ceil(signal_freq * N / 65_000_000)
    return signal_freq * N / cycles


def spectrum(freq, physical_sample_rate, square=False, hann=False):
    samples = []
    for index in range(N):
        value = math.sin(2 * math.pi * freq * (index + 0.25) / physical_sample_rate)
        if square:
            value = 1.0 if value >= 0.0 else -1.0
        samples.append(value)

    mean = sum(samples) / N
    window = ([0.5 * (1 - math.cos(2 * math.pi * i / (N - 1)))
               for i in range(N)] if hann else [1.0] * N)
    magnitude = []
    for bin_index in range(N // 2):
        step = cmath.exp(-2j * math.pi * bin_index / N)
        phase = 1.0 + 0.0j
        total = 0.0j
        for sample, coefficient in zip(samples, window):
            total += (sample - mean) * coefficient * phase
            phase *= step
        magnitude.append(abs(total) * 2.0 / sum(window))
    return magnitude


def peak_frequency(magnitude, sample_rate):
    peak_bin = max(range(1, N // 2), key=magnitude.__getitem__)
    offset = 0.0
    if 1 < peak_bin < N // 2 - 1:
        left, center, right = magnitude[peak_bin - 1 : peak_bin + 2]
        denominator = left - 2.0 * center + right
        if abs(denominator) >= 1e-10:
            offset = max(-0.5, min(0.5, 0.5 * (left - right) / denominator))
    return (peak_bin + offset) * sample_rate / N


def thd(magnitude, sample_rate):
    fundamental = peak_frequency(magnitude, sample_rate)
    resolution = sample_rate / N
    fundamental_bin = int(fundamental / resolution + 0.5)
    fundamental_power = magnitude[fundamental_bin] ** 2
    harmonic_power = 0.0

    for harmonic in range(2, 16):
        center = int(harmonic * fundamental / resolution + 0.5)
        if center >= N // 2:
            break
        harmonic_power += magnitude[center] ** 2

    highest_harmonic = min(15, int((sample_rate * 0.5 - resolution) / fundamental))
    value = math.sqrt(harmonic_power / fundamental_power) * 100.0
    return value, highest_harmonic


def thd_n(magnitude, sample_rate):
    fundamental = peak_frequency(magnitude, sample_rate)
    fundamental_bin = round(fundamental * N / sample_rate)
    fundamental_power = magnitude[fundamental_bin] ** 2
    total_power = sum(value * value for value in magnitude[1:N // 2])
    return math.sqrt(max(0.0, total_power - fundamental_power) /
                     fundamental_power) * 100.0


def band_power(magnitude, center, radius=2):
    first = max(1, center - radius)
    last = min(N // 2 - 1, center + radius)
    return sum(magnitude[index] ** 2 for index in range(first, last + 1))


def robust_thd_n(magnitude, sample_rate):
    fundamental = peak_frequency(magnitude, sample_rate)
    fundamental_bin = round(fundamental * N / sample_rate)
    fundamental_power = band_power(magnitude, fundamental_bin)
    total_power = sum(value * value for value in magnitude[1:N // 2])
    return math.sqrt(max(0.0, total_power - fundamental_power) /
                     fundamental_power) * 100.0


def robust_thd(magnitude, sample_rate):
    fundamental = peak_frequency(magnitude, sample_rate)
    fundamental_bin = round(fundamental * N / sample_rate)
    fundamental_power = band_power(magnitude, fundamental_bin)
    harmonic_power = 0.0
    for harmonic in range(2, 16):
        harmonic_bin = round(harmonic * fundamental * N / sample_rate)
        if harmonic_bin >= N // 2:
            break
        harmonic_power += band_power(magnitude, harmonic_bin)
    return math.sqrt(harmonic_power / fundamental_power) * 100.0


if __name__ == "__main__":
    low_freq = 5_000
    low_rate = formal_sample_rate(low_freq)
    high_freq = 500_000
    high_rate = formal_sample_rate(high_freq)

    sine_5k, sine_5k_max = thd(spectrum(low_freq, low_rate), low_rate)
    square_5k, square_5k_max = thd(spectrum(low_freq, low_rate, True), low_rate)
    square_500k, square_500k_max = thd(spectrum(high_freq, high_rate, True), high_rate)

    measured_50k = 49_866
    rate_50k = formal_sample_rate(measured_50k)
    sine_50k_error, _ = thd(spectrum(50_000, rate_50k), rate_50k)
    measured_500k = 499_907
    rate_500k = formal_sample_rate(measured_500k)
    sine_500k_error, _ = thd(spectrum(500_000, rate_500k), rate_500k)

    board_misdetections = (
        (5_000, 926.555),
        (50_000, 2_345.941),
        (500_000, 164_799.297),
        (5_000_000, 3_757_097.750),
    )
    refined = [
        peak_frequency(spectrum(actual, formal_sample_rate(coarse)),
                       formal_sample_rate(coarse))
        for actual, coarse in board_misdetections
    ]
    noncoherent_5m_thd_n = thd_n(spectrum(5_000_000, 65_000_000),
                                 65_000_000)
    robust_5m_thd_n = robust_thd_n(
        spectrum(5_000_000, 65_000_000, hann=True), 65_000_000)
    robust_square_5k = robust_thd(spectrum(5_000, low_rate, square=True,
                                          hann=True), low_rate)
    robust_square_500k = robust_thd(spectrum(500_000, high_rate, square=True,
                                            hann=True), high_rate)
    fft_source = (ROOT / "zuolan_stm32/MY_Algorithms/Src/my_fft.c").read_text(
        encoding="utf-8")

    assert sine_5k_max == 15 and sine_5k < 1e-6
    assert square_5k_max == 15 and 45.0 < square_5k < 45.3
    assert square_500k_max == 15 and 45.4 < square_500k < 45.8
    assert sine_50k_error < 0.6
    assert sine_500k_error < 0.1
    assert all(abs(value - actual) / actual < 0.01
               for value, (actual, _) in zip(refined, board_misdetections))
    assert 40.0 < noncoherent_5m_thd_n < 50.0, noncoherent_5m_thd_n
    assert robust_5m_thd_n < 2.0, robust_5m_thd_n
    assert 45.0 < robust_square_5k < 45.3, robust_square_5k
    assert 45.4 < robust_square_500k < 45.8, robust_square_500k
    assert "sumSpectrumBandPower" in fft_source
    coherent_5m_rate = formal_sample_rate(5_000_000)
    assert abs(N * 5_000_000 / coherent_5m_rate - 79) < 1e-9

    print(f"5k正弦 THD={sine_5k:.3f}%")
    print(f"5k方波 THD={square_5k:.3f}%")
    print(f"500k方波直接采样 THD={square_500k:.3f}%")
    print(f"50k实测频差回放 THD={sine_50k_error:.3f}%")
    print(f"500k实测频差回放 THD={sine_500k_error:.3f}%")
    print(f"5M非相干THD+N: 旧算法={noncoherent_5m_thd_n:.3f}%, "
          f"Hann主瓣积分={robust_5m_thd_n:.3f}%")
