"""G题采集、滤波、量程和HMI静态设计检查。"""

import cmath
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


def response(coefficients, frequency_hz, sample_rate_hz):
    value = sum(
        coefficient
        * cmath.exp(-2j * math.pi * frequency_hz * index / sample_rate_hz)
        for index, coefficient in enumerate(coefficients)
    )
    return abs(value)


def main():
    common = read(STM32 / "MY_Utilities/Inc/commond_init.h")
    algorithm = read(STM32 / "MY_Algorithms/Src/g_signal_analysis.c")
    app = read(STM32 / "MY_APP/g_signal_app.c")
    app_header = read(STM32 / "MY_APP/g_signal_app.h")
    usart = read(STM32 / "MY_Communication/Src/my_usart.c")
    hmi = read(STM32 / "MY_Communication/Src/my_hmi.c")
    top = read(FPGA / "src/TOP.v")
    fifo = read(FPGA / "src/G_AD_FIFO.v")

    capture_size = macro_number(common, "G_CAPTURE_SIZE")
    fft_size = macro_number(common, "G_FFT_SIZE")
    decimation = macro_number(common, "G_DECIMATION")
    sample_rate = macro_number(common, "G_SAMPLE_RATE_HZ")
    assert capture_size == 16384
    assert fft_size == 4096
    assert decimation == 4
    assert sample_rate == 5_000_000
    assert capture_size == fft_size * decimation
    assert sample_rate / decimation / fft_size <= 500.0
    resolution = sample_rate / decimation / fft_size
    assert abs(resolution - 305.17578125) < 1e-9
    first_bin = math.floor(10_000.0 / resolution)
    assert first_bin * resolution <= 10_000.0
    for maximum_frequency in (200_000.0, 500_000.0):
        last_bin = math.ceil(maximum_frequency / resolution)
        assert last_bin * resolution >= maximum_frequency

    half_match = re.search(
        r"fir_half\[G_FIR_HALF_TAPS\]\s*=\s*\{(.*?)\};",
        algorithm,
        re.DOTALL,
    )
    assert half_match
    half = [
        float(value)
        for value in re.findall(
            r"[-+]?\d+(?:\.\d*)?(?:e[-+]?\d+)?(?=f)",
            half_match.group(1),
            re.IGNORECASE,
        )
    ]
    assert len(half) == 101
    coefficients = half[:-1] + [half[-1]] + half[-2::-1]
    assert abs(sum(coefficients) - 1.0) < 1e-6
    assert 20.0 * math.log10(
        response(coefficients, 500_000.0, sample_rate)
    ) > -0.1
    assert 20.0 * math.log10(
        response(coefficients, 625_000.0, sample_rate)
    ) < -60.0
    assert 20.0 * math.log10(
        response(coefficients, 650_000.0, sample_rate)
    ) < -60.0
    assert 20.0 * math.log10(
        response(coefficients, 1_000_000.0, sample_rate)
    ) < -80.0

    assert "bin = first_bin; bin <= last_bin" in algorithm
    assert "floorf(G_MIN_FREQUENCY_HZ / resolution)" in algorithm
    assert "ceilf(max_frequency / resolution)" in algorithm
    assert "logf(fmaxf(spectrum_magnitude" in algorithm
    assert "G_SPECTRUM_AXIS_BASELINE" in algorithm
    assert "output[0] = G_SPECTRUM_AXIS_TOP" in algorithm
    assert "G_SPECTRUM_DATA_FIRST_POINT" in algorithm
    assert "static float findRisingMidpointTime" in algorithm
    assert "float start_time = findRisingMidpointTime(result)" in algorithm
    assert "start_time + periods * point" in algorithm
    assert "output[points - 1U] = output[0]" in algorithm

    amplitudes = (0.060, 0.030, 0.020)
    expected_rms = math.sqrt(sum(value * value for value in amplitudes) / 2.0)
    assert abs(expected_rms - 0.0494974747) < 1e-9
    waveform = [
        sum(
            amplitude * math.sin(2.0 * math.pi * harmonic * point / 4096.0)
            for harmonic, amplitude in enumerate(amplitudes, start=1)
        )
        for point in range(4096)
    ]
    assert 0.12 < max(waveform) - min(waveform) < 0.22

    assert abs(macro_number(app, "G_MODE1_AMPLITUDE_CALIBRATION") - 1.10756) < 1e-6
    assert abs(macro_number(app, "G_MODE2_AMPLITUDE_CALIBRATION") - 1.143) < 1e-6
    assert abs(macro_number(app, "G_MODE3_AMPLITUDE_CALIBRATION") - 1.143) < 1e-6
    assert macro_number(app, "G_MEASUREMENT_FRAME_COUNT") == 3
    assert macro_number(app, "G_VCA_MAX_ADJUSTMENTS") == 3
    minimum_code = macro_number(app, "G_VCA_DAC_MIN_CODE")
    maximum_code = macro_number(app, "G_VCA_DAC_MAX_CODE")
    minimum_gain = macro_number(app, "G_VCA_CALIBRATED_GAIN_MIN")
    maximum_gain = macro_number(app, "G_VCA_CALIBRATED_GAIN_MAX")
    assert minimum_code == 2053
    assert maximum_code == 2463
    assert abs(minimum_gain - 6.324) < 1e-6
    assert abs(maximum_gain - 28.339) < 1e-6
    assert macro_number(app, "G_VCA_ADJUST_LOW_VPP") == 0.70
    assert macro_number(app, "G_VCA_TARGET_ADC_VPP") == 0.90
    assert macro_number(app, "G_VCA_ADJUST_HIGH_VPP") == 1.10
    assert macro_number(app, "G_VCA_LOW_GAIN_CAL_10KHZ_HZ") == 10_000.0
    assert macro_number(app, "G_VCA_LOW_GAIN_CAL_30KHZ_HZ") == 30_000.0
    assert macro_number(app, "G_VCA_LOW_GAIN_CAL_50KHZ_HZ") == 50_000.0
    assert macro_number(app, "G_VCA_LOW_GAIN_CAL_START_HZ") == 100_000.0
    assert macro_number(app, "G_VCA_LOW_GAIN_CAL_END_HZ") == 300_000.0
    assert abs(
        macro_number(app, "G_VCA_LOW_GAIN_CAL_10KHZ_SCALE") - 0.959221
    ) < 1e-6
    assert abs(
        macro_number(app, "G_VCA_LOW_GAIN_CAL_30KHZ_SCALE") - 0.999856
    ) < 1e-6
    assert abs(
        macro_number(app, "G_VCA_LOW_GAIN_CAL_50KHZ_SCALE") - 0.981869
    ) < 1e-6
    low_gain_start_scale = macro_number(
        app, "G_VCA_LOW_GAIN_CAL_START_SCALE"
    )
    assert abs(low_gain_start_scale - 1.030962) < 1e-6
    assert macro_number(app, "G_VCA_LOW_GAIN_CAL_END_SCALE") == 1.0

    assert "G_APP_REFRESH_MS" not in app
    assert "last_measurement_ms" not in app
    assert macro_number(app, "G_HMI_PAGE_READY_DELAY_MS") >= 30
    assert "static void deferPageRefresh(void)" in app
    assert "page_refresh_pending = 1U" in app
    assert "(int32_t)(HAL_GetTick() - page_refresh_due_ms) >= 0" in app
    assert "if (page_refresh_pending)" in app
    assert "HAL_Delay(G_HMI_PAGE_READY_DELAY_MS)" not in app
    assert macro_number(app, "G_HMI_PAGE_POLL_MS") <= 200
    assert '"sendme\\xff\\xff\\xff"' in app
    assert "static void startCaptureSamplePrint" in app
    assert "static void printCaptureSamplesTask(void)" in app
    assert "sample_print_index++" in app
    assert "G_SAMPLE_ABORT,NEW_MEASUREMENT" in app
    assert "pending_events &= ~(G_EVENT_RESULT_PAGE | G_EVENT_WAVE_PAGE)" in app
    assert 'clearResults("READY")' in app
    assert "G_SAMPLE_BEGIN" in app
    assert "INDEX,ANALYSIS12" in app
    assert "&huart1" in app
    assert "static uint16_t active_gain_dac_code" in app
    assert "static float active_calibrated_gain" in app
    assert "static uint8_t gain_programmed" in app
    assert "static float calibratedGainForDacCode" in app
    assert "static uint16_t adjustedGainDacCode" in app
    assert "static float interpolateFrequencyScale" in app
    assert "static float lowGainFrequencyScale" in app
    assert "static void applyLowGainFrequencyCalibration" in app
    assert "gain_dac_code != G_VCA_DAC_MIN_CODE" in app
    assert "GSignal_RecalculateMetrics(result)" in app
    assert "float code_change = logf(ratio)" in app
    assert "if (!gain_programmed || gain_dac_code != active_gain_dac_code)" in app
    assert "adc_vpp >= G_VCA_ADJUST_LOW_VPP" in app
    assert "adc_vpp <= G_VCA_ADJUST_HIGH_VPP" in app
    assert "adjustment < G_VCA_MAX_ADJUSTMENTS" in app
    assert "adjusted_code == gain_dac_code" in app
    assert "static uint8_t medianMeasurementIndex" in app
    assert "medianMeasurementIndex(valid_frame_count)" in app
    assert "G_SIGNAL_CONTROL_CLEAR" in app_header
    assert "G_EVENT_CLEAR = 1U << 6" in app
    assert "G_SAMPLE_ABORT,CLEAR" in app
    assert "sample_print_active = 0U" in app
    assert "memset(&measurement, 0, sizeof(measurement))" in app
    assert 'clearResults("READY")' in app
    for command in (
        "0x31", "0x32", "0x33", "0x34", "0x35", "0x36", "0x37"
    ):
        assert command in usart
    assert "case 0x66" in usart
    assert "hmiPageResponseBytes = 4U" in usart
    assert "HMI_HandlePageId(hmiPageResponseId)" in usart
    for component in (
        "tMode", "tPeriod", "tView", "tStatus", "nFreq", "nVpp",
        "nVrms", "nRes",
        "nX0", "nX1", "nX2", "nY0", "nY1", "nY2", "tAxisX", "tAxisY",
    ):
        assert f'"{component}"' in app
    assert "char frequency_name[4]" in app
    assert "char amplitude_name[4]" in app
    assert "(char)('1' + index)" in app
    assert "HAL_UART_Transmit" in hmi
    assert '"cle %s.id,%d\\xff\\xff\\xff"' in hmi
    assert "\\x01\\xff\\xff\\xff" not in hmi

    assert "reg [13:0] ad1_capture_count" in top
    assert "14'd16383" in top
    assert "G_AD_FIFO ad1_fifo" in top
    assert "lpm_numwords = 16384" in fifo
    assert macro_number(app, "G_ADC_CAPTURE_ATTEMPTS") == 2
    assert "G_ADC_ERROR,GAIN_DAC=%u,ATTEMPTS=%u" in app
    print("G signal design checks passed")


if __name__ == "__main__":
    main()
