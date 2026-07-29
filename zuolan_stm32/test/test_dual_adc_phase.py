"""双 AD 同步相位测量最小回归：仅使用 Python 标准库。"""

import cmath
import math
from pathlib import Path


N = 1024
ROOT = Path(__file__).resolve().parents[2]


def fft_phase(samples, sample_rate, signal_freq):
    mean = sum(samples) / len(samples)
    window = [0.5 * (1.0 - math.cos(2.0 * math.pi * i / (N - 1)))
              for i in range(N)]
    target_bin = round(signal_freq * N / sample_rate)
    coefficient = sum(
        (sample - mean) * weight
        * cmath.exp(-2j * math.pi * target_bin * index / N)
        for index, (sample, weight) in enumerate(zip(samples, window))
    )
    return math.degrees(cmath.phase(coefficient))


def phase_difference(phase1, phase2, signal_freq, sampling_skew_deg=0.0,
                     calibration_ns=0.0):
    calibration_deg = calibration_ns * 1e-9 * signal_freq * 360.0
    difference = phase2 - phase1 - sampling_skew_deg - calibration_deg
    while difference > 180.0:
        difference -= 360.0
    while difference <= -180.0:
        difference += 360.0
    return difference


if __name__ == "__main__":
    phase1 = 20.0
    phase2 = 110.0
    measured = 0.0
    for signal_freq in (5_000.0, 50_000.0, 500_000.0, 5_000_000.0):
        target_rate = signal_freq * (128.0 if signal_freq <= 100_000.0 else 64.0)
        if target_rate > 65_000_000.0:
            cycles = math.ceil(signal_freq * N / 65_000_000.0)
            target_rate = signal_freq * N / cycles
        samples1 = [math.sin(2.0 * math.pi * signal_freq * i / target_rate
                             + math.radians(phase1)) for i in range(N)]
        samples2 = [math.sin(
            2.0 * math.pi * signal_freq
            * (i / target_rate + 0.5 / target_rate)
            + math.radians(phase2)) for i in range(N)]
        sampling_skew_deg = signal_freq * 180.0 / target_rate
        measured = phase_difference(
            fft_phase(samples1, target_rate, signal_freq),
            fft_phase(samples2, target_rate, signal_freq), signal_freq,
            sampling_skew_deg,
        )
        assert abs(measured - 90.0) < 0.1, (signal_freq, measured)

    assert phase_difference(170.0, -170.0, 5_000.0) == 20.0
    assert phase_difference(-170.0, 170.0, 5_000.0) == -20.0
    calibration_ns = 5.0 / (360.0 * 1_000_000.0) * 1e9
    assert abs(phase_difference(0.0, 95.0, 1_000_000.0,
                                calibration_ns=calibration_ns) - 90.0) < 1e-6

    top = (ROOT / "zuolan_FPGA/src/TOP.v").read_text(encoding="utf-8")
    nco = (ROOT / "zuolan_FPGA/src/FREQ_DEV.v").read_text(encoding="utf-8")
    control = (ROOT / "zuolan_stm32/MY_Utilities/Inc/commond_init.h").read_text(
        encoding="utf-8")
    adc = (ROOT / "zuolan_stm32/MY_Hardware_Drivers/Src/ad_measure.c").read_text(
        encoding="utf-8")
    fft = (ROOT / "zuolan_stm32/MY_Algorithms/Src/my_fft.c").read_text(
        encoding="utf-8")

    assert "AD_SYNC_MODE = (1 << 1)" in control
    assert "assign sync_phase_load = control_data[1] & ~sync_mode_d;" in top
    assert "assign ad2_sampling_clock = ad2_sampling_clock_nco;" in top
    assert ".PHASE_INIT(32'h00000000)" in top
    assert ".PHASE_INIT(32'h80000000)" in top
    assert "input             PHASE_LOAD" in nco
    assert "ACC <= PHASE_INIT;" in nco
    assert "sync_capture_started" in top
    assert "ad2_capture_strobe" in top
    assert "resetFifoPair();" in adc
    assert "AD_FIFO_WRITE_ENABLE(3);" in adc
    assert "sampling_skew_deg = signal_freq * 180.0f" in adc
    assert "fft_get_phase" in fft

    print(f"Dual ADC phase contract passed: AD2-AD1={measured:.3f} deg")
