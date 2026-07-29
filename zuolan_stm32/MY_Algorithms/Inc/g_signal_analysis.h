#ifndef G_SIGNAL_ANALYSIS_H
#define G_SIGNAL_ANALYSIS_H

#include <stdint.h>

#define G_SIGNAL_MAX_COMPONENTS 3U
#define G_SIGNAL_HMI_POINTS 400U

typedef enum
{
    G_SIGNAL_MODE_200KHZ = 0,
    G_SIGNAL_MODE_500KHZ,
    G_SIGNAL_MODE_INTERFERENCE,
    G_SIGNAL_MODE_COUNT
} GSignalMode;

typedef struct
{
    float frequency_hz;
    float amplitude_v;
    float phase_rad;
    uint16_t harmonic;
} GSignalComponent;

typedef struct
{
    uint8_t valid;
    uint8_t component_count;
    float fundamental_hz;
    float vpp_v;
    float vrms_v;
    float frequency_resolution_hz;
    GSignalComponent components[G_SIGNAL_MAX_COMPONENTS];
} GSignalResult;

int GSignal_Analyze(const float *samples, uint32_t sample_count,
                    GSignalMode mode, GSignalResult *result);
void GSignal_RecalculateMetrics(GSignalResult *result);
void GSignal_BuildTimeWave(const GSignalResult *result, uint8_t periods,
                           uint8_t *output, uint16_t points);
void GSignal_BuildSpectrumWave(const GSignalResult *result, GSignalMode mode,
                               uint8_t *output, uint16_t points);
float GSignal_ModeMaxFrequency(GSignalMode mode);

#endif
