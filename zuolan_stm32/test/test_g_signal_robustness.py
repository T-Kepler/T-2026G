"""Regression checks for G-problem ADC impulses and false harmonics."""

import math
import re
from pathlib import Path


STM32 = Path(__file__).resolve().parents[1]
FPGA = STM32.parent / "zuolan_FPGA"


def read(path):
    return path.read_text(encoding="utf-8")


def macro_number(source, name):
    match = re.search(rf"#define\s+{name}\s+([0-9.]+)", source)
    assert match, f"missing {name}"
    return float(match.group(1))


def repair_impulses(raw, threshold, neighbor_limit):
    repaired = list(raw)
    repair_count = 0
    for index in range(1, len(raw) - 1):
        left = raw[index - 1]
        center = raw[index]
        right = raw[index + 1]
        midpoint = (left + right + 1) // 2
        if (abs(center - midpoint) >= threshold and
                abs(left - right) <= neighbor_limit):
            repaired[index] = midpoint
            repair_count += 1
    return repaired, repair_count


def sine_codes(frequency_hz, sample_rate_hz, count, amplitude_codes):
    return [
        round(2048 + amplitude_codes * math.sin(
            2.0 * math.pi * frequency_hz * index / sample_rate_hz
        ))
        for index in range(count)
    ]


def select_family(candidates, tolerance_hz, minimum_ratio, dominant_ratio):
    candidates = sorted(candidates, key=lambda item: item[1], reverse=True)
    maximum = max(amplitude for _, amplitude in candidates)
    best = None
    for base_frequency, base_amplitude in candidates:
        family = [(1, base_frequency, base_amplitude)]
        for frequency, amplitude in candidates:
            if frequency <= base_frequency:
                continue
            harmonic = round(frequency / base_frequency)
            if not 2 <= harmonic <= 50:
                continue
            if abs(frequency - harmonic * base_frequency) > tolerance_hz:
                continue
            if any(item[0] == harmonic for item in family):
                continue
            family.append((harmonic, frequency, amplitude))
        family_maximum = max(item[2] for item in family)
        if base_amplitude < family_maximum * minimum_ratio:
            continue
        has_dominant = family_maximum >= maximum * dominant_ratio
        score = sum(item[2] * item[2] for item in family)
        rank = (has_dominant, len(family),
                -base_frequency if len(family) > 1 else score)
        if best is None or rank > best[0]:
            report = [family[0]] + sorted(
                family[1:], key=lambda item: item[2], reverse=True
            )[:2]
            best = (rank, base_frequency,
                    sorted(item[0] for item in report))
    return best[1], best[2]


def main():
    app = read(STM32 / "MY_APP/g_signal_app.c")
    analysis = read(STM32 / "MY_Algorithms/Src/g_signal_analysis.c")
    adc = read(STM32 / "MY_Hardware_Drivers/Src/ad_measure.c")
    top = read(FPGA / "src/TOP.v")

    threshold = int(macro_number(app, "G_ADC_IMPULSE_THRESHOLD_CODES"))
    neighbor_limit = int(macro_number(
        app, "G_ADC_IMPULSE_NEIGHBOR_LIMIT_CODES"
    ))
    assert 400 <= threshold <= 800
    assert neighbor_limit >= 4

    raw = sine_codes(10_000.0, 5_000_000.0, 16_384, 200)
    corrupted = list(raw)
    bad_indices = list(range(95, len(raw) - 1, 500))
    for index in bad_indices:
        corrupted[index] = 3913
    repaired, repair_count = repair_impulses(
        corrupted, threshold, neighbor_limit
    )
    assert repair_count == len(bad_indices)
    assert max(abs(actual - expected)
               for actual, expected in zip(repaired, raw)) <= 1

    highest_frequency = sine_codes(
        500_000.0, 5_000_000.0, 4096, 2047
    )
    unchanged, repair_count = repair_impulses(
        highest_frequency, threshold, neighbor_limit
    )
    assert repair_count == 0
    assert unchanged == highest_frequency

    candidate_threshold = macro_number(
        analysis, "G_COMPONENT_ABSOLUTE_THRESHOLD_V"
    )
    report_threshold = macro_number(
        analysis, "G_COMPONENT_REPORT_THRESHOLD_V"
    )
    family_tolerance_bins = macro_number(
        analysis, "G_FAMILY_TOLERANCE_BINS"
    )
    minimum_ratio = macro_number(
        analysis, "G_FUNDAMENTAL_FAMILY_MIN_RATIO"
    )
    dominant_ratio = macro_number(analysis, "G_DOMINANT_FAMILY_RATIO")
    assert 0.0008 <= candidate_threshold <= 0.002
    assert candidate_threshold <= report_threshold <= 0.005
    assert 2.0 <= family_tolerance_bins <= 3.0
    assert 0.015 <= minimum_ratio <= 0.03
    assert 0.5 <= dominant_ratio <= 1.0

    candidates = [
        (42_000.0, 0.220),
        (31_500.0, 0.184),
        (84_000.0, 0.012),
        (126_000.0, 0.010),
        (10_500.0, 0.006),
    ]
    selected, harmonics = select_family(
        candidates,
        family_tolerance_bins * (1_250_000.0 / 4096.0),
        minimum_ratio,
        dominant_ratio,
    )
    assert selected == 10_500.0
    assert harmonics == [1, 3, 4]

    measured_distorted = [
        (126_037.6, 0.372),
        (83_923.3, 0.306),
        (168_151.9, 0.104),
        (42_114.3, 0.044),
    ]
    selected, harmonics = select_family(
        measured_distorted,
        family_tolerance_bins * (1_250_000.0 / 4096.0),
        minimum_ratio,
        dominant_ratio,
    )
    assert abs(selected - 42_114.3) < 1.0
    assert harmonics == [1, 2, 3]

    selected, harmonics = select_family(
        [(42_000.0, 0.220), (84_000.0, 0.180),
         (126_000.0, 0.150), (10_500.0, 0.003)],
        family_tolerance_bins * (1_250_000.0 / 4096.0),
        minimum_ratio,
        dominant_ratio,
    )
    assert selected == 42_000.0
    assert harmonics == [1, 2, 3]

    selected, harmonics = select_family(
        [(10_500.0, 0.070), (31_500.0, 0.035)],
        family_tolerance_bins * (1_250_000.0 / 4096.0),
        minimum_ratio,
        dominant_ratio,
    )
    assert selected == 10_500.0
    assert harmonics == [1, 3]

    selected, harmonics = select_family(
        [(20_000.0, 0.080), (80_000.0, 0.025)],
        family_tolerance_bins * (1_250_000.0 / 4096.0),
        minimum_ratio,
        dominant_ratio,
    )
    assert selected == 20_000.0
    assert harmonics == [1, 4]

    assert select_family(
        [(42_000.0, 0.220)],
        family_tolerance_bins * (1_250_000.0 / 4096.0),
        minimum_ratio,
        dominant_ratio,
    ) == (42_000.0, [1])
    assert "static uint32_t repairCaptureImpulses" in app
    assert "return (raw_code - G_ADC_MIDPOINT_CODE)" in app
    assert "return (raw_code - ADC_SCALE)" in adc
    assert "adcCodeToVoltage(fifo_data[i])" in adc
    assert "adcCodeToVoltage(raw)" in adc
    assert "ANALYSIS_MIN" in app
    assert "REPAIRED" in app
    assert "capture_raw[index] = analysis_code" in app
    assert "INDEX,ANALYSIS12" in app
    assert "family_count < G_PEAK_CANDIDATES" in analysis
    assert "best_family_count" in analysis
    assert "pruneWeakComponents" in analysis
    assert "reg [11:0] ad1_input_pipe1" in top
    assert ".data  (ad1_input_pipe2)" in top
    print("G signal robustness checks passed")


if __name__ == "__main__":
    main()
