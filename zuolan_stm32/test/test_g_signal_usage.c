#include "ad_measure.h"
#include "g_signal_analysis.h"

static float g_frame[G_CAPTURE_SIZE];
static uint16_t g_raw[G_CAPTURE_SIZE];
static uint8_t g_wave[G_SIGNAL_HMI_POINTS];

void example_g_signal_analysis(void)
{
    uint16_t raw_minimum;
    uint16_t raw_maximum;
    GSignalResult result;

    if (!ad_capture_g_frame(G_SAMPLE_RATE_HZ, g_frame, g_raw,
                            &raw_minimum, &raw_maximum))
        return;
    if (!GSignal_Analyze(g_frame, G_CAPTURE_SIZE,
                         G_SIGNAL_MODE_500KHZ, &result))
        return;

    GSignal_RecalculateMetrics(&result);
    GSignal_BuildTimeWave(&result, 3U, g_wave, G_SIGNAL_HMI_POINTS);
    GSignal_BuildSpectrumWave(&result, G_SIGNAL_MODE_500KHZ,
                              g_wave, G_SIGNAL_HMI_POINTS);
    (void)GSignal_ModeMaxFrequency(G_SIGNAL_MODE_500KHZ);
}
