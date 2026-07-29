#include "g_signal_app.h"

#include "ad_measure.h"
#include "dac.h"
#include "my_hmi.h"
#include "my_usart.h"
#include "usart.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define G_VCA_SETTLE_MS 300U
#define G_ADC_CAPTURE_ATTEMPTS 2U
#define G_VCA_MAX_ADJUSTMENTS 3U
#define G_MEASUREMENT_FRAME_COUNT 3U
#define G_SAMPLE_PRINT_BUFFER_SIZE 192U
#define G_SAMPLE_PRINT_LINE_SPACE 24U
#define G_SAMPLE_PRINT_TIMEOUT_MS 1000U
#define G_HMI_PAGE_READY_DELAY_MS 50U
#define G_HMI_PAGE_POLL_MS 100U
#define G_ADC_SATURATION_LOW 32U
#define G_ADC_SATURATION_HIGH 4063U
#define G_MINIMUM_VALID_VPP 0.02f
#define G_ADC_IMPULSE_THRESHOLD_CODES 512U
#define G_ADC_IMPULSE_NEIGHBOR_LIMIT_CODES 512U
#define G_ADC_MIDPOINT_CODE 2048.0f
#define G_ADC_INPUT_HALF_RANGE_V 5.0f

#define G_VCA_DAC_MIN_CODE 2053U
#define G_VCA_DAC_MAX_CODE 2463U
#define G_VCA_CALIBRATED_GAIN_MIN 6.324f
#define G_VCA_CALIBRATED_GAIN_MAX 28.339f
#define G_VCA_TARGET_ADC_VPP 0.90f
#define G_VCA_ADJUST_LOW_VPP 0.70f
#define G_VCA_ADJUST_HIGH_VPP 1.10f
#define G_VCA_LOW_GAIN_CAL_10KHZ_HZ 10000.0f
#define G_VCA_LOW_GAIN_CAL_30KHZ_HZ 30000.0f
#define G_VCA_LOW_GAIN_CAL_50KHZ_HZ 50000.0f
#define G_VCA_LOW_GAIN_CAL_START_HZ 100000.0f
#define G_VCA_LOW_GAIN_CAL_END_HZ 300000.0f
#define G_VCA_LOW_GAIN_CAL_10KHZ_SCALE 0.959221f
#define G_VCA_LOW_GAIN_CAL_30KHZ_SCALE 0.999856f
#define G_VCA_LOW_GAIN_CAL_50KHZ_SCALE 0.981869f
#define G_VCA_LOW_GAIN_CAL_START_SCALE 1.030962f
#define G_VCA_LOW_GAIN_CAL_END_SCALE 1.000000f

#define G_MODE1_AMPLITUDE_CALIBRATION 1.10756f
#define G_MODE2_AMPLITUDE_CALIBRATION 1.143f
#define G_MODE3_AMPLITUDE_CALIBRATION 1.143f

typedef enum
{
    G_VIEW_TIME = 0,
    G_VIEW_SPECTRUM
} GViewMode;

typedef enum
{
    G_HMI_PAGE_RESULT = 0,
    G_HMI_PAGE_WAVE
} GHmiPage;

enum
{
    G_EVENT_MODE = 1U << 0,
    G_EVENT_PERIOD = 1U << 1,
    G_EVENT_VIEW = 1U << 2,
    G_EVENT_MEASURE = 1U << 3,
    G_EVENT_RESULT_PAGE = 1U << 4,
    G_EVENT_WAVE_PAGE = 1U << 5,
    G_EVENT_CLEAR = 1U << 6
};

static float capture_samples[G_CAPTURE_SIZE];
static uint16_t capture_raw[G_CAPTURE_SIZE];
static uint8_t hmi_wave[G_SIGNAL_HMI_POINTS];
static GSignalResult measurement;
static GSignalResult measurement_frames[G_MEASUREMENT_FRAME_COUNT];
static volatile uint32_t pending_events;
static GSignalMode current_mode;
static GViewMode current_view;
static uint8_t display_periods;
static uint8_t redraw_pending;
static uint8_t page_refresh_pending;
static uint32_t page_refresh_due_ms;
static uint32_t last_hmi_page_poll_ms;
static uint32_t sample_print_index;
static uint8_t sample_print_active;
static uint16_t sample_print_gain_dac_code;
static uint32_t sample_print_gain_x1000;
static uint16_t sample_print_raw_minimum;
static uint16_t sample_print_raw_maximum;
static uint16_t sample_print_analysis_minimum;
static uint16_t sample_print_analysis_maximum;
static uint32_t sample_print_repaired_count;
static uint16_t capture_physical_raw_minimum;
static uint16_t capture_physical_raw_maximum;
static uint32_t capture_repaired_count;
static uint16_t active_gain_dac_code;
static float active_calibrated_gain;
static uint8_t gain_programmed;
static GHmiPage active_hmi_page;
static const char *display_status;

static uint32_t takeEvents(void)
{
    uint32_t interrupt_state = __get_PRIMASK();
    __disable_irq();
    uint32_t events = pending_events;
    pending_events = 0U;
    if (interrupt_state == 0U)
        __enable_irq();
    return events;
}

static void deferPageRefresh(void)
{
    page_refresh_due_ms = HAL_GetTick() + G_HMI_PAGE_READY_DELAY_MS;
    page_refresh_pending = 1U;
    redraw_pending = 1U;
}

static void updatePageRefreshDelay(void)
{
    if (page_refresh_pending &&
        (int32_t)(HAL_GetTick() - page_refresh_due_ms) >= 0)
        page_refresh_pending = 0U;
}

static void pollHmiPage(void)
{
    uint32_t now = HAL_GetTick();
    if (now - last_hmi_page_poll_ms < G_HMI_PAGE_POLL_MS)
        return;

    last_hmi_page_poll_ms = now;
    my_printf(&huart2, "sendme\xff\xff\xff");
}

static int setGainDacCode(uint16_t dac_code)
{
    if (HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R,
                         dac_code) != HAL_OK ||
        HAL_DAC_Start(&hdac, DAC_CHANNEL_2) != HAL_OK)
        return 0;
    HAL_Delay(G_VCA_SETTLE_MS);
    return 1;
}

static float calibratedGainForDacCode(uint16_t dac_code)
{
    float position = (dac_code - G_VCA_DAC_MIN_CODE) /
                     (float)(G_VCA_DAC_MAX_CODE - G_VCA_DAC_MIN_CODE);
    float gain_ratio = G_VCA_CALIBRATED_GAIN_MAX /
                       G_VCA_CALIBRATED_GAIN_MIN;
    return G_VCA_CALIBRATED_GAIN_MIN * expf(logf(gain_ratio) * position);
}

static uint16_t adjustedGainDacCode(uint16_t current_code, float adc_vpp)
{
    if (adc_vpp <= G_MINIMUM_VALID_VPP)
        return G_VCA_DAC_MAX_CODE;

    float ratio = G_VCA_TARGET_ADC_VPP / adc_vpp;
    float code_change = logf(ratio) /
                        logf(G_VCA_CALIBRATED_GAIN_MAX /
                             G_VCA_CALIBRATED_GAIN_MIN) *
                        (G_VCA_DAC_MAX_CODE - G_VCA_DAC_MIN_CODE);
    int32_t code = current_code + (int32_t)(code_change +
                   ((code_change >= 0.0f) ? 0.5f : -0.5f));
    if (code < (int32_t)G_VCA_DAC_MIN_CODE)
        code = G_VCA_DAC_MIN_CODE;
    if (code > (int32_t)G_VCA_DAC_MAX_CODE)
        code = G_VCA_DAC_MAX_CODE;
    return (uint16_t)code;
}

static float modeAmplitudeCalibration(void)
{
    static const float calibration[G_SIGNAL_MODE_COUNT] = {
        G_MODE1_AMPLITUDE_CALIBRATION,
        G_MODE2_AMPLITUDE_CALIBRATION,
        G_MODE3_AMPLITUDE_CALIBRATION
    };
    return calibration[current_mode];
}

static float interpolateFrequencyScale(float frequency_hz,
                                       float lower_frequency_hz,
                                       float lower_scale,
                                       float upper_frequency_hz,
                                       float upper_scale)
{
    float position = (frequency_hz - lower_frequency_hz) /
                     (upper_frequency_hz - lower_frequency_hz);
    return lower_scale + (upper_scale - lower_scale) * position;
}

static float lowGainFrequencyScale(float frequency_hz)
{
    if (frequency_hz <= G_VCA_LOW_GAIN_CAL_10KHZ_HZ)
        return G_VCA_LOW_GAIN_CAL_10KHZ_SCALE;
    if (frequency_hz <= G_VCA_LOW_GAIN_CAL_30KHZ_HZ)
        return interpolateFrequencyScale(
            frequency_hz, G_VCA_LOW_GAIN_CAL_10KHZ_HZ,
            G_VCA_LOW_GAIN_CAL_10KHZ_SCALE,
            G_VCA_LOW_GAIN_CAL_30KHZ_HZ,
            G_VCA_LOW_GAIN_CAL_30KHZ_SCALE);
    if (frequency_hz <= G_VCA_LOW_GAIN_CAL_50KHZ_HZ)
        return interpolateFrequencyScale(
            frequency_hz, G_VCA_LOW_GAIN_CAL_30KHZ_HZ,
            G_VCA_LOW_GAIN_CAL_30KHZ_SCALE,
            G_VCA_LOW_GAIN_CAL_50KHZ_HZ,
            G_VCA_LOW_GAIN_CAL_50KHZ_SCALE);
    if (frequency_hz <= G_VCA_LOW_GAIN_CAL_START_HZ)
        return interpolateFrequencyScale(
            frequency_hz, G_VCA_LOW_GAIN_CAL_50KHZ_HZ,
            G_VCA_LOW_GAIN_CAL_50KHZ_SCALE,
            G_VCA_LOW_GAIN_CAL_START_HZ,
            G_VCA_LOW_GAIN_CAL_START_SCALE);
    if (frequency_hz >= G_VCA_LOW_GAIN_CAL_END_HZ)
        return G_VCA_LOW_GAIN_CAL_END_SCALE;
    return interpolateFrequencyScale(
        frequency_hz, G_VCA_LOW_GAIN_CAL_START_HZ,
        G_VCA_LOW_GAIN_CAL_START_SCALE,
        G_VCA_LOW_GAIN_CAL_END_HZ,
        G_VCA_LOW_GAIN_CAL_END_SCALE);
}

static void applyLowGainFrequencyCalibration(uint16_t gain_dac_code,
                                             GSignalResult *result)
{
    if (gain_dac_code != G_VCA_DAC_MIN_CODE)
        return;

    for (uint8_t index = 0U; index < result->component_count; index++)
        result->components[index].amplitude_v *= lowGainFrequencyScale(
            result->components[index].frequency_hz);
    GSignal_RecalculateMetrics(result);
}

static const char *modeText(void)
{
    static const char *const text[G_SIGNAL_MODE_COUNT] = {
        "1: 10-200kHz",
        "2: 10-500kHz",
        "3: INTERFERENCE"
    };
    return text[current_mode];
}

static int spectrumMaximumAmplitudeMv(void)
{
    float maximum_amplitude_v = 0.0f;
    for (uint8_t index = 0U; index < measurement.component_count; index++)
        if (measurement.components[index].amplitude_v > maximum_amplitude_v)
            maximum_amplitude_v = measurement.components[index].amplitude_v;
    return (int)(maximum_amplitude_v * 1000.0f + 0.999f);
}

static void updateSpectrumAxes(void)
{
    static const char *const controls[] = {
        "nX0", "nX1", "nX2", "nY0", "nY1", "nY2", "tAxisX", "tAxisY"
    };
    int visible = (current_view == G_VIEW_SPECTRUM) ? 1 : 0;
    for (uint8_t index = 0U;
         index < sizeof(controls) / sizeof(controls[0]); index++)
        my_printf(&huart2, "vis %s,%d\xff\xff\xff", controls[index], visible);

    if (!visible)
        return;

    int maximum_frequency_khz =
        (int)(GSignal_ModeMaxFrequency(current_mode) / 1000.0f + 0.5f);
    int maximum_amplitude_mv = spectrumMaximumAmplitudeMv();
    HMI_Send_Int(huart2, "nX0", 0);
    HMI_Send_Int(huart2, "nX1", maximum_frequency_khz / 2);
    HMI_Send_Int(huart2, "nX2", maximum_frequency_khz);
    HMI_Send_Int(huart2, "nY0", 0);
    HMI_Send_Int(huart2, "nY1", (maximum_amplitude_mv + 1) / 2);
    HMI_Send_Int(huart2, "nY2", maximum_amplitude_mv);
}

static void sendOperatingState(void)
{
    HMI_Send_String(huart2, "tMode", modeText());
    HMI_Send_String(huart2, "tPeriod",
                    (display_periods == 1U) ? "1 PERIOD" : "3 PERIODS");
    HMI_Send_String(huart2, "tView",
                    (current_view == G_VIEW_TIME) ? "TIME" : "SPECTRUM");
}

static void sendMeasurementValues(void)
{
    HMI_Send_String(huart2, "tStatus", display_status);
    HMI_Send_Int(huart2, "nFreq",
                 measurement.valid ? (int)(measurement.fundamental_hz + 0.5f) : 0);
    HMI_Send_Int(huart2, "nVpp",
                 measurement.valid ? (int)(measurement.vpp_v * 1000.0f + 0.5f) : 0);
    HMI_Send_Int(huart2, "nVrms",
                 measurement.valid ? (int)(measurement.vrms_v * 1000.0f + 0.5f) : 0);
    HMI_Send_Int(huart2, "nRes",
                 measurement.valid ?
                 (int)(measurement.frequency_resolution_hz + 0.5f) : 0);

    for (uint8_t index = 0U; index < G_SIGNAL_MAX_COMPONENTS; index++)
    {
        char frequency_name[4] = {'n', 'F', (char)('1' + index), '\0'};
        char amplitude_name[4] = {'n', 'A', (char)('1' + index), '\0'};
        int frequency_hz = 0;
        int amplitude_mv = 0;
        if (measurement.valid && index < measurement.component_count)
        {
            frequency_hz = (int)(measurement.components[index].frequency_hz +
                                 0.5f);
            amplitude_mv = (int)(measurement.components[index].amplitude_v *
                                 1000.0f + 0.5f);
        }
        HMI_Send_Int(huart2, frequency_name, frequency_hz);
        HMI_Send_Int(huart2, amplitude_name, amplitude_mv);
    }
}

static void sendWaveform(void)
{
    if (current_view == G_VIEW_TIME)
        GSignal_BuildTimeWave(&measurement, display_periods, hmi_wave,
                              G_SIGNAL_HMI_POINTS);
    else
        GSignal_BuildSpectrumWave(&measurement, current_mode, hmi_wave,
                                  G_SIGNAL_HMI_POINTS);

    HMI_Wave_Clear(huart2, "s0", 0);
    HMI_Write_Wave_Fast(huart2, "s0", 0, G_SIGNAL_HMI_POINTS, hmi_wave);
}

static void refreshResultPage(void)
{
    sendOperatingState();
    sendMeasurementValues();
}

static void refreshWavePage(void)
{
    sendOperatingState();
    updateSpectrumAxes();
    if (measurement.valid)
        sendWaveform();
    else
        HMI_Wave_Clear(huart2, "s0", 0);
}

static void refreshActivePage(void)
{
    if (page_refresh_pending)
    {
        redraw_pending = 1U;
        return;
    }

    if (active_hmi_page == G_HMI_PAGE_WAVE)
        refreshWavePage();
    else
        refreshResultPage();
}

static void clearResults(const char *status)
{
    display_status = status;
    refreshActivePage();
}

static void sendResults(void)
{
    if (!measurement.valid)
        display_status = "NO SIGNAL";
    else
        display_status = "OK";
    refreshActivePage();
}

static float captureCodeToVoltage(uint16_t raw_code)
{
    return (raw_code - G_ADC_MIDPOINT_CODE) * G_ADC_INPUT_HALF_RANGE_V /
           G_ADC_MIDPOINT_CODE;
}

static uint32_t repairCaptureImpulses(uint16_t *analysis_minimum,
                                      uint16_t *analysis_maximum)
{
    uint16_t minimum = 0x0FFFU;
    uint16_t maximum = 0U;
    uint32_t repaired_count = 0U;

    for (uint32_t index = 0U; index < G_CAPTURE_SIZE; index++)
    {
        uint16_t analysis_code = capture_raw[index];
        if (index > 0U && index + 1U < G_CAPTURE_SIZE)
        {
            uint32_t left = capture_raw[index - 1U];
            uint32_t center = capture_raw[index];
            uint32_t right = capture_raw[index + 1U];
            uint32_t midpoint = (left + right + 1U) / 2U;
            uint32_t deviation = (center > midpoint) ?
                                 center - midpoint : midpoint - center;
            uint32_t neighbor_delta = (left > right) ?
                                      left - right : right - left;

            if (deviation >= G_ADC_IMPULSE_THRESHOLD_CODES &&
                neighbor_delta <= G_ADC_IMPULSE_NEIGHBOR_LIMIT_CODES)
            {
                analysis_code = (uint16_t)midpoint;
                capture_samples[index] = captureCodeToVoltage(analysis_code);
                repaired_count++;
            }
        }

        capture_raw[index] = analysis_code;
        if (analysis_code < minimum) minimum = analysis_code;
        if (analysis_code > maximum) maximum = analysis_code;
    }

    *analysis_minimum = minimum;
    *analysis_maximum = maximum;
    return repaired_count;
}

static void startCaptureSamplePrint(uint16_t gain_dac_code,
                                    float calibrated_gain,
                                    uint16_t raw_minimum,
                                    uint16_t raw_maximum)
{
    sample_print_index = 0U;
    sample_print_active = 1U;
    sample_print_gain_dac_code = gain_dac_code;
    sample_print_gain_x1000 = (uint32_t)(calibrated_gain * 1000.0f + 0.5f);
    sample_print_raw_minimum = capture_physical_raw_minimum;
    sample_print_raw_maximum = capture_physical_raw_maximum;
    sample_print_analysis_minimum = raw_minimum;
    sample_print_analysis_maximum = raw_maximum;
    sample_print_repaired_count = capture_repaired_count;
    my_printf(&huart1,
              "G_SAMPLE_BEGIN,FS_HZ=%lu,COUNT=%lu,DECIMATION=%u,FFT_FS_HZ=%lu,FFT_COUNT=%lu,GAIN_DAC=%u,GAIN_X1000=%lu,RAW_MIN=%u,RAW_MAX=%u,ANALYSIS_MIN=%u,ANALYSIS_MAX=%u,REPAIRED=%lu\r\n",
              (unsigned long)G_SAMPLE_RATE_HZ,
              (unsigned long)G_CAPTURE_SIZE,
              (unsigned int)G_DECIMATION,
              (unsigned long)(G_SAMPLE_RATE_HZ / G_DECIMATION),
              (unsigned long)G_FFT_SIZE,
               (unsigned int)sample_print_gain_dac_code,
               (unsigned long)sample_print_gain_x1000,
               (unsigned int)sample_print_raw_minimum,
               (unsigned int)sample_print_raw_maximum,
               (unsigned int)sample_print_analysis_minimum,
               (unsigned int)sample_print_analysis_maximum,
               (unsigned long)sample_print_repaired_count);
    my_printf(&huart1, "INDEX,ANALYSIS12\r\n");
}

static void printCaptureSamplesTask(void)
{
    if (!sample_print_active)
        return;

    char buffer[G_SAMPLE_PRINT_BUFFER_SIZE];
    size_t used = 0U;

    while (sample_print_index < G_CAPTURE_SIZE &&
           sizeof(buffer) - used >= G_SAMPLE_PRINT_LINE_SPACE)
    {
        size_t available = sizeof(buffer) - used;
        int written = snprintf(buffer + used, available, "%lu,%u\r\n",
                               (unsigned long)sample_print_index,
                               (unsigned int)capture_raw[sample_print_index]);
        if (written < 0 || (size_t)written >= available)
        {
            sample_print_active = 0U;
            my_printf(&huart1, "G_SAMPLE_PRINT_ERROR\r\n");
            return;
        }
        used += (size_t)written;
        sample_print_index++;
    }

    if (used > 0U &&
        HAL_UART_Transmit(&huart1, (uint8_t *)buffer, (uint16_t)used,
                          G_SAMPLE_PRINT_TIMEOUT_MS) != HAL_OK)
    {
        sample_print_active = 0U;
        return;
    }

    if (sample_print_index >= G_CAPTURE_SIZE)
    {
        sample_print_active = 0U;
        my_printf(&huart1, "G_SAMPLE_END\r\n");
        my_printf(&huart1,
                  "G_SAMPLE_SUMMARY,GAIN_DAC=%u,GAIN_X1000=%lu,RAW_MIN=%u,RAW_MAX=%u,RAW_SPAN=%u,ANALYSIS_MIN=%u,ANALYSIS_MAX=%u,ANALYSIS_SPAN=%u,REPAIRED=%lu\r\n",
                  (unsigned int)sample_print_gain_dac_code,
                  (unsigned long)sample_print_gain_x1000,
                  (unsigned int)sample_print_raw_minimum,
                  (unsigned int)sample_print_raw_maximum,
                  (unsigned int)(sample_print_raw_maximum -
                                 sample_print_raw_minimum),
                  (unsigned int)sample_print_analysis_minimum,
                  (unsigned int)sample_print_analysis_maximum,
                  (unsigned int)(sample_print_analysis_maximum -
                                 sample_print_analysis_minimum),
                  (unsigned long)sample_print_repaired_count);
    }
}

static int captureAtGain(uint16_t gain_dac_code, uint16_t *raw_minimum,
                         uint16_t *raw_maximum)
{
    if (!gain_programmed || gain_dac_code != active_gain_dac_code)
    {
        if (!setGainDacCode(gain_dac_code))
        {
            gain_programmed = 0U;
            return 0;
        }
        active_gain_dac_code = gain_dac_code;
        active_calibrated_gain = calibratedGainForDacCode(gain_dac_code);
        gain_programmed = 1U;
    }
    for (uint8_t attempt = 0U; attempt < G_ADC_CAPTURE_ATTEMPTS; attempt++)
        if (ad_capture_g_frame(G_SAMPLE_RATE_HZ, capture_samples, capture_raw,
                               &capture_physical_raw_minimum,
                               &capture_physical_raw_maximum))
        {
            capture_repaired_count = repairCaptureImpulses(raw_minimum,
                                                            raw_maximum);
            return 1;
        }

    my_printf(&huart1, "G_ADC_ERROR,GAIN_DAC=%u,ATTEMPTS=%u\r\n",
              (unsigned int)gain_dac_code,
              (unsigned int)G_ADC_CAPTURE_ATTEMPTS);
    return 0;
}

static int analyzeCapture(uint16_t gain_dac_code, float calibrated_gain,
                          GSignalResult *result)
{
    float input_scale = modeAmplitudeCalibration() / calibrated_gain;
    for (uint32_t index = 0U; index < G_CAPTURE_SIZE; index++)
        capture_samples[index] *= input_scale;

    if (!GSignal_Analyze(capture_samples, G_CAPTURE_SIZE, current_mode,
                         result))
        return 0;
    applyLowGainFrequencyCalibration(gain_dac_code, result);
    return result->vpp_v >= G_MINIMUM_VALID_VPP;
}

static uint8_t medianMeasurementIndex(uint8_t count)
{
    uint8_t indices[G_MEASUREMENT_FRAME_COUNT] = {0U, 1U, 2U};
    for (uint8_t index = 1U; index < count; index++)
    {
        uint8_t current = indices[index];
        uint8_t position = index;
        while (position > 0U &&
               measurement_frames[indices[position - 1U]].vpp_v >
               measurement_frames[current].vpp_v)
        {
            indices[position] = indices[position - 1U];
            position--;
        }
        indices[position] = current;
    }
    return indices[count / 2U];
}

static void performMeasurement(void)
{
    uint16_t raw_minimum;
    uint16_t raw_maximum;
    uint16_t gain_dac_code = active_gain_dac_code;
    float calibrated_gain = active_calibrated_gain;

    if (sample_print_active)
        my_printf(&huart1, "G_SAMPLE_ABORT,NEW_MEASUREMENT\r\n");
    sample_print_active = 0U;
    display_status = "MEASURING";
    if (active_hmi_page == G_HMI_PAGE_RESULT)
    {
        sendOperatingState();
        HMI_Send_String(huart2, "tStatus", display_status);
    }
    memset(&measurement, 0, sizeof(measurement));

    if (!captureAtGain(gain_dac_code, &raw_minimum, &raw_maximum))
    {
        clearResults("ADC ERROR");
        return;
    }

    for (uint8_t adjustment = 0U;
         adjustment < G_VCA_MAX_ADJUSTMENTS; adjustment++)
    {
        float adc_vpp = (raw_maximum - raw_minimum) *
                        G_ADC_INPUT_HALF_RANGE_V / G_ADC_MIDPOINT_CODE;
        if (adc_vpp >= G_VCA_ADJUST_LOW_VPP &&
            adc_vpp <= G_VCA_ADJUST_HIGH_VPP)
            break;

        uint16_t adjusted_code = adjustedGainDacCode(gain_dac_code, adc_vpp);
        if (adjusted_code == gain_dac_code)
            break;

        gain_dac_code = adjusted_code;
        if (!captureAtGain(gain_dac_code, &raw_minimum, &raw_maximum))
        {
            clearResults("ADC ERROR");
            return;
        }
    }

    if ((raw_minimum <= G_ADC_SATURATION_LOW ||
         raw_maximum >= G_ADC_SATURATION_HIGH) &&
        gain_dac_code != G_VCA_DAC_MIN_CODE)
    {
        gain_dac_code = G_VCA_DAC_MIN_CODE;
        if (!captureAtGain(gain_dac_code, &raw_minimum, &raw_maximum))
        {
            clearResults("ADC ERROR");
            return;
        }
    }

    if (raw_minimum <= G_ADC_SATURATION_LOW ||
        raw_maximum >= G_ADC_SATURATION_HIGH)
    {
        clearResults("OVERLOAD");
        calibrated_gain = calibratedGainForDacCode(gain_dac_code);
        startCaptureSamplePrint(gain_dac_code, calibrated_gain,
                                raw_minimum, raw_maximum);
        return;
    }

    calibrated_gain = calibratedGainForDacCode(gain_dac_code);
    uint8_t valid_frame_count = 0U;
    for (uint8_t frame = 0U; frame < G_MEASUREMENT_FRAME_COUNT; frame++)
    {
        if (frame > 0U &&
            !captureAtGain(gain_dac_code, &raw_minimum, &raw_maximum))
            continue;
        if (raw_minimum <= G_ADC_SATURATION_LOW ||
            raw_maximum >= G_ADC_SATURATION_HIGH)
            continue;
        if (analyzeCapture(gain_dac_code, calibrated_gain,
                           &measurement_frames[valid_frame_count]))
            valid_frame_count++;
    }

    if (valid_frame_count == 0U)
        memset(&measurement, 0, sizeof(measurement));
    else
        measurement = measurement_frames[
            medianMeasurementIndex(valid_frame_count)];
    sendResults();
    startCaptureSamplePrint(gain_dac_code, calibrated_gain,
                            raw_minimum, raw_maximum);
}

void GSignalApp_Init(void)
{
    current_mode = G_SIGNAL_MODE_200KHZ;
    current_view = G_VIEW_TIME;
    display_periods = 1U;
    pending_events = 0U;
    redraw_pending = 0U;
    page_refresh_pending = 0U;
    page_refresh_due_ms = 0U;
    last_hmi_page_poll_ms = HAL_GetTick();
    sample_print_index = 0U;
    sample_print_active = 0U;
    sample_print_gain_dac_code = G_VCA_DAC_MAX_CODE;
    sample_print_gain_x1000 =
        (uint32_t)(G_VCA_CALIBRATED_GAIN_MAX * 1000.0f + 0.5f);
    sample_print_raw_minimum = 0U;
    sample_print_raw_maximum = 0U;
    sample_print_analysis_minimum = 0U;
    sample_print_analysis_maximum = 0U;
    sample_print_repaired_count = 0U;
    capture_physical_raw_minimum = 0U;
    capture_physical_raw_maximum = 0U;
    capture_repaired_count = 0U;
    active_gain_dac_code = G_VCA_DAC_MAX_CODE;
    active_calibrated_gain = G_VCA_CALIBRATED_GAIN_MAX;
    gain_programmed = 0U;
    active_hmi_page = G_HMI_PAGE_RESULT;
    display_status = "READY";
    memset(&measurement, 0, sizeof(measurement));
    if (setGainDacCode(G_VCA_DAC_MAX_CODE))
        gain_programmed = 1U;
    refreshResultPage();
}

void GSignalApp_HandleControl(GSignalControl control)
{
    if (control <= G_SIGNAL_CONTROL_CLEAR)
    {
        uint32_t interrupt_state = __get_PRIMASK();
        __disable_irq();
        if (control == G_SIGNAL_CONTROL_RESULT_PAGE_READY ||
            control == G_SIGNAL_CONTROL_WAVE_PAGE_READY)
            pending_events &= ~(G_EVENT_RESULT_PAGE | G_EVENT_WAVE_PAGE);
        pending_events |= 1UL << control;
        if (interrupt_state == 0U)
            __enable_irq();
    }
}

void GSignalApp_Task(void)
{
    pollHmiPage();
    uint32_t events = takeEvents();
    updatePageRefreshDelay();
    if ((events & G_EVENT_RESULT_PAGE) != 0U)
    {
        active_hmi_page = G_HMI_PAGE_RESULT;
        deferPageRefresh();
    }
    if ((events & G_EVENT_WAVE_PAGE) != 0U)
    {
        active_hmi_page = G_HMI_PAGE_WAVE;
        deferPageRefresh();
    }

    if ((events & G_EVENT_CLEAR) != 0U)
    {
        if (sample_print_active)
            my_printf(&huart1, "G_SAMPLE_ABORT,CLEAR\r\n");
        sample_print_active = 0U;
        sample_print_index = 0U;
        memset(&measurement, 0, sizeof(measurement));
        memset(measurement_frames, 0, sizeof(measurement_frames));
        memset(hmi_wave, 0, sizeof(hmi_wave));
        redraw_pending = 0U;
        clearResults("READY");
        return;
    }

    if ((events & G_EVENT_MODE) != 0U)
    {
        current_mode = (GSignalMode)((current_mode + 1U) % G_SIGNAL_MODE_COUNT);
        memset(&measurement, 0, sizeof(measurement));
    }
    if ((events & G_EVENT_PERIOD) != 0U)
    {
        display_periods = (display_periods == 1U) ? 3U : 1U;
        redraw_pending = 1U;
    }
    if ((events & G_EVENT_VIEW) != 0U)
    {
        current_view = (current_view == G_VIEW_TIME) ?
                       G_VIEW_SPECTRUM : G_VIEW_TIME;
        redraw_pending = 1U;
    }
    if ((events & G_EVENT_MEASURE) != 0U)
    {
        redraw_pending = 0U;
        performMeasurement();
        printCaptureSamplesTask();
        return;
    }

    if ((events & G_EVENT_MODE) != 0U)
    {
        redraw_pending = 0U;
        clearResults("READY");
    }
    else if ((events & (G_EVENT_RESULT_PAGE | G_EVENT_WAVE_PAGE)) != 0U ||
             redraw_pending)
    {
        redraw_pending = 0U;
        refreshActivePage();
    }
    printCaptureSamplesTask();
}
