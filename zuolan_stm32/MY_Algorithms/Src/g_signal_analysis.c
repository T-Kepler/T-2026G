#include "g_signal_analysis.h"

#include "arm_const_structs.h"
#include "arm_math.h"
#include "commond_init.h"
#include <math.h>
#include <string.h>

#define G_PI 3.14159265358979323846f
#define G_FIR_HALF_TAPS 101U
#define G_PEAK_CANDIDATES 32U
#define G_MIN_FREQUENCY_HZ 10000.0f
#define G_FAMILY_TOLERANCE_BINS 3.0f
#define G_SPECTRUM_AXIS_BASELINE 8U
#define G_SPECTRUM_AXIS_TOP 247U
#define G_SPECTRUM_DATA_FIRST_POINT 2U
#define G_TIME_WAVE_ALIGNMENT_STEPS 1024U
#define G_COMPONENT_ABSOLUTE_THRESHOLD_V 0.001f
#define G_COMPONENT_RELATIVE_THRESHOLD 0.005f
#define G_COMPONENT_REPORT_THRESHOLD_V 0.004f
#define G_FUNDAMENTAL_FAMILY_MIN_RATIO 0.02f
#define G_DOMINANT_FAMILY_RATIO 0.80f

typedef struct
{
    uint16_t bin;
    float magnitude;
    float frequency_hz;
} SpectrumCandidate;

static float fft_buffer[G_FFT_SIZE * 2U];
static float spectrum_magnitude[G_FFT_SIZE / 2U];

static const float fir_half[G_FIR_HALF_TAPS] = {
    8.070234895655e-06f, 5.994940516924e-06f, -2.226567544200e-06f,
    -1.414298927471e-05f, -2.325099132487e-05f, -2.186616710886e-05f,
    -5.982859575861e-06f, 2.050953448498e-05f, 4.532123057846e-05f,
    5.256366981802e-05f, 3.149773238987e-05f, -1.507813891523e-05f,
    -6.816101852804e-05f, -9.894733444744e-05f, -8.302260530411e-05f,
    -1.597111468538e-05f, 7.850469799800e-05f, 1.547106135484e-04f,
    1.653882386678e-04f, 8.815349093914e-05f, -5.580686998184e-05f,
    -2.032727894434e-04f, -2.745172397409e-04f, -2.137784373789e-04f,
    -2.524268558494e-05f, 2.164750514498e-04f, 3.924507743013e-04f,
    3.951615261297e-04f, 1.890229126180e-04f, -1.564463089703e-04f,
    -4.841019181616e-04f, -6.172008415645e-04f, -4.503509744076e-04f,
    -1.841151894833e-05f, 4.975556122545e-04f, 8.412664870931e-04f,
    8.041365888831e-04f, 3.432096313094e-04f, -3.694029742285e-04f,
    -1.002706380483e-03f, -1.215470044759e-03f, -8.331544373141e-04f,
    3.574345754527e-05f, 1.014086678400e-03f, 1.612896918059e-03f,
    1.469124272979e-03f, 5.517751149654e-04f, -7.754780286491e-04f,
    -1.887657068682e-03f, -2.185328094652e-03f, -1.406919888708e-03f,
    1.917498486945e-04f, 1.900964625599e-03f, 2.862414600599e-03f,
    2.490071239958e-03f, 8.047991831430e-04f, -1.500004375898e-03f,
    -3.328841312089e-03f, -3.693454345258e-03f, -2.229662862080e-03f,
    5.414376630765e-04f, 3.371837990177e-03f, 4.833790666840e-03f,
    4.023305838457e-03f, 1.080766465721e-03f, -2.756999999989e-03f,
    -5.654102007825e-03f, -6.034218702516e-03f, -3.407957345453e-03f,
    1.252599742643e-03f, 5.833256921459e-03f, 8.009954404077e-03f,
    6.395438211583e-03f, 1.348916799500e-03f, -4.996593490560e-03f,
    -9.593943978100e-03f, -9.902300456417e-03f, -5.225102234779e-03f,
    2.711190621459e-03f, 1.031662053910e-02f, 1.369608696159e-02f,
    1.055794842856e-02f, 1.574360741213e-03f, -9.545735692539e-03f,
    -1.747337896107e-02f, -1.767136715906e-02f, -8.764574849719e-03f,
    6.279842169421e-03f, 2.089436233034e-02f, 2.750006833184e-02f,
    2.100285494645e-02f, 1.724889093113e-03f, -2.362652199896e-02f,
    -4.364648261546e-02f, -4.619242925827e-02f, -2.322153693285e-02f,
    2.539048837773e-02f, 9.068704783559e-02f, 1.568069001354e-01f,
    2.058905330028e-01f, 2.240019754826e-01f
};

float GSignal_ModeMaxFrequency(GSignalMode mode)
{
    return (mode == G_SIGNAL_MODE_200KHZ) ? 200000.0f : 500000.0f;
}

static float firFilteredSample(const float *samples, uint32_t sample_count,
                               uint32_t center)
{
    float value = fir_half[G_FIR_HALF_TAPS - 1U] * samples[center];
    for (uint32_t tap = 0; tap < G_FIR_HALF_TAPS - 1U; tap++)
    {
        uint32_t offset = G_FIR_HALF_TAPS - 1U - tap;
        if (center >= offset)
            value += fir_half[tap] * samples[center - offset];
        if (center + offset < sample_count)
            value += fir_half[tap] * samples[center + offset];
    }
    return value;
}

static void prepareSpectrum(const float *samples, uint32_t sample_count)
{
    float mean = 0.0f;
    for (uint32_t i = 0; i < G_FFT_SIZE; i++)
    {
        float value = firFilteredSample(samples, sample_count,
                                        i * G_DECIMATION);
        fft_buffer[2U * i] = value;
        mean += value;
    }
    mean /= G_FFT_SIZE;

    float window_sum = 0.0f;
    for (uint32_t i = 0; i < G_FFT_SIZE; i++)
    {
        float window = 0.5f - 0.5f * cosf(2.0f * G_PI * i /
                                         (G_FFT_SIZE - 1U));
        fft_buffer[2U * i] = (fft_buffer[2U * i] - mean) * window;
        fft_buffer[2U * i + 1U] = 0.0f;
        window_sum += window;
    }

    arm_cfft_f32(&arm_cfft_sR_f32_len4096, fft_buffer, 0, 1);
    for (uint32_t i = 0; i < G_FFT_SIZE / 2U; i++)
    {
        float real = fft_buffer[2U * i];
        float imag = fft_buffer[2U * i + 1U];
        spectrum_magnitude[i] = 2.0f * sqrtf(real * real + imag * imag) /
                                window_sum;
    }
}

static void insertCandidate(SpectrumCandidate *candidates, uint8_t *count,
                            SpectrumCandidate candidate)
{
    uint8_t position = *count;
    if (position < G_PEAK_CANDIDATES)
        (*count)++;
    else if (candidate.magnitude <= candidates[position - 1U].magnitude)
        return;
    else
        position--;

    while (position > 0U &&
           candidate.magnitude > candidates[position - 1U].magnitude)
    {
        if (position < G_PEAK_CANDIDATES)
            candidates[position] = candidates[position - 1U];
        position--;
    }
    candidates[position] = candidate;
}

static float refineFrequency(uint16_t bin, float frequency_resolution)
{
    float left = logf(fmaxf(spectrum_magnitude[bin - 1U], 1e-30f));
    float center = logf(fmaxf(spectrum_magnitude[bin], 1e-30f));
    float right = logf(fmaxf(spectrum_magnitude[bin + 1U], 1e-30f));
    float denominator = left - 2.0f * center + right;
    float delta = 0.0f;
    if (fabsf(denominator) > 1e-20f)
        delta = 0.5f * (left - right) / denominator;
    if (delta > 0.5f) delta = 0.5f;
    if (delta < -0.5f) delta = -0.5f;
    return (bin + delta) * frequency_resolution;
}

static uint8_t findHarmonicFamily(float max_frequency,
                                  GSignalComponent *components,
                                  float *fundamental_hz)
{
    float analysis_rate = G_SAMPLE_RATE_HZ / G_DECIMATION;
    float resolution = analysis_rate / G_FFT_SIZE;
    uint16_t first_bin = (uint16_t)floorf(G_MIN_FREQUENCY_HZ / resolution);
    uint16_t last_bin = (uint16_t)ceilf(max_frequency / resolution);
    if (first_bin < 1U) first_bin = 1U;
    if (last_bin >= G_FFT_SIZE / 2U - 1U)
        last_bin = G_FFT_SIZE / 2U - 2U;
    float maximum = 0.0f;
    float noise_sum = 0.0f;
    uint32_t noise_count = 0U;

    for (uint16_t bin = first_bin; bin <= last_bin; bin++)
        if (spectrum_magnitude[bin] > maximum)
            maximum = spectrum_magnitude[bin];
    if (maximum <= 0.0f)
        return 0U;

    for (uint16_t bin = first_bin; bin <= last_bin; bin++)
    {
        if (spectrum_magnitude[bin] < maximum * 0.02f)
        {
            noise_sum += spectrum_magnitude[bin];
            noise_count++;
        }
    }
    float noise = (noise_count > 0U) ? noise_sum / noise_count : 0.0f;
    float threshold = fmaxf(noise * 8.0f,
                            maximum * G_COMPONENT_RELATIVE_THRESHOLD);
    threshold = fmaxf(threshold, G_COMPONENT_ABSOLUTE_THRESHOLD_V);

    SpectrumCandidate candidates[G_PEAK_CANDIDATES];
    uint8_t candidate_count = 0U;

    for (uint16_t bin = first_bin; bin <= last_bin; bin++)
    {
        float value = spectrum_magnitude[bin];
        if (value >= threshold && value > spectrum_magnitude[bin - 1U] &&
            value >= spectrum_magnitude[bin + 1U])
        {
            SpectrumCandidate candidate;
            candidate.bin = bin;
            candidate.magnitude = value;
            candidate.frequency_hz = refineFrequency(bin, resolution);
            insertCandidate(candidates, &candidate_count, candidate);
        }
    }
    if (candidate_count == 0U)
        return 0U;

    uint8_t best_family_indices[G_PEAK_CANDIDATES] = {0U};
    uint16_t best_family_harmonics[G_PEAK_CANDIDATES] = {0U};
    uint8_t best_family_count = 0U;
    uint8_t best_has_dominant = 0U;
    float best_score = 0.0f;
    float best_base_frequency = 0.0f;

    for (uint8_t base = 0U; base < candidate_count; base++)
    {
        uint8_t family_indices[G_PEAK_CANDIDATES] = {0U};
        uint16_t family_harmonics[G_PEAK_CANDIDATES] = {0U};
        float family_magnitudes[G_PEAK_CANDIDATES] = {0.0f};
        family_indices[0] = base;
        family_harmonics[0] = 1U;
        family_magnitudes[0] = candidates[base].magnitude;
        uint8_t family_count = 1U;

        for (uint8_t index = 0U; index < candidate_count; index++)
        {
            if (index == base ||
                candidates[index].frequency_hz <= candidates[base].frequency_hz)
                continue;
            float ratio_f = candidates[index].frequency_hz /
                            candidates[base].frequency_hz;
            uint16_t harmonic = (uint16_t)(ratio_f + 0.5f);
            if (harmonic < 2U || harmonic > 50U)
                continue;
            float error = fabsf(candidates[index].frequency_hz -
                                harmonic * candidates[base].frequency_hz);
            if (error > G_FAMILY_TOLERANCE_BINS * resolution)
                continue;

            uint8_t slot = family_count;
            for (uint8_t existing = 1U; existing < family_count; existing++)
            {
                if (family_harmonics[existing] == harmonic)
                {
                    slot = existing;
                    break;
                }
            }
            if (slot < family_count)
            {
                if (candidates[index].magnitude > family_magnitudes[slot])
                {
                    family_indices[slot] = index;
                    family_magnitudes[slot] = candidates[index].magnitude;
                }
            }
            else if (family_count < G_PEAK_CANDIDATES)
            {
                family_indices[family_count] = index;
                family_harmonics[family_count] = harmonic;
                family_magnitudes[family_count] = candidates[index].magnitude;
                family_count++;
            }
        }

        float family_maximum = 0.0f;
        float family_score = 0.0f;
        for (uint8_t i = 0U; i < family_count; i++)
        {
            if (family_magnitudes[i] > family_maximum)
                family_maximum = family_magnitudes[i];
            family_score += family_magnitudes[i] * family_magnitudes[i];
        }
        if (family_magnitudes[0] <
            family_maximum * G_FUNDAMENTAL_FAMILY_MIN_RATIO)
            continue;

        uint8_t family_has_dominant =
            (family_maximum >= maximum * G_DOMINANT_FAMILY_RATIO) ? 1U : 0U;
        uint8_t replace_best = 0U;
        if (best_family_count == 0U ||
            family_has_dominant > best_has_dominant)
            replace_best = 1U;
        else if (family_has_dominant == best_has_dominant)
        {
            if (family_count > best_family_count)
                replace_best = 1U;
            else if (family_count == best_family_count && family_count > 1U &&
                     candidates[base].frequency_hz < best_base_frequency)
                replace_best = 1U;
            else if (family_count == best_family_count && family_count == 1U &&
                     family_score > best_score)
                replace_best = 1U;
        }

        if (replace_best)
        {
            best_family_count = family_count;
            best_has_dominant = family_has_dominant;
            best_score = family_score;
            best_base_frequency = candidates[base].frequency_hz;
            for (uint8_t i = 0U; i < family_count; i++)
            {
                best_family_indices[i] = family_indices[i];
                best_family_harmonics[i] = family_harmonics[i];
            }
        }
    }

    if (best_family_count == 0U)
        return 0U;

    float weighted_frequency = 0.0f;
    float weight_sum = 0.0f;
    for (uint8_t i = 0U; i < best_family_count; i++)
    {
        SpectrumCandidate *candidate =
            &candidates[best_family_indices[i]];
        float weight = candidate->magnitude * candidate->magnitude;
        weighted_frequency += weight * candidate->frequency_hz /
                              best_family_harmonics[i];
        weight_sum += weight;
    }
    *fundamental_hz = weighted_frequency / weight_sum;

    uint8_t report_count = best_family_count;
    if (report_count > G_SIGNAL_MAX_COMPONENTS)
        report_count = G_SIGNAL_MAX_COMPONENTS;
    for (uint8_t i = 0U; i < report_count; i++)
    {
        components[i].harmonic = best_family_harmonics[i];
        components[i].frequency_hz =
            *fundamental_hz * best_family_harmonics[i];
        components[i].amplitude_v = 0.0f;
        components[i].phase_rad = 0.0f;
    }
    for (uint8_t i = 0U; i + 1U < report_count; i++)
    {
        for (uint8_t j = i + 1U; j < report_count; j++)
        {
            if (components[j].harmonic < components[i].harmonic)
            {
                GSignalComponent temporary = components[i];
                components[i] = components[j];
                components[j] = temporary;
            }
        }
    }
    return report_count;
}

static int solveLinearSystem(float matrix[7][8], uint8_t size)
{
    for (uint8_t column = 0U; column < size; column++)
    {
        uint8_t pivot = column;
        for (uint8_t row = column + 1U; row < size; row++)
            if (fabsf(matrix[row][column]) > fabsf(matrix[pivot][column]))
                pivot = row;
        if (fabsf(matrix[pivot][column]) < 1e-12f)
            return 0;
        if (pivot != column)
        {
            for (uint8_t item = column; item <= size; item++)
            {
                float temporary = matrix[column][item];
                matrix[column][item] = matrix[pivot][item];
                matrix[pivot][item] = temporary;
            }
        }

        float divisor = matrix[column][column];
        for (uint8_t item = column; item <= size; item++)
            matrix[column][item] /= divisor;
        for (uint8_t row = 0U; row < size; row++)
        {
            if (row == column) continue;
            float factor = matrix[row][column];
            for (uint8_t item = column; item <= size; item++)
                matrix[row][item] -= factor * matrix[column][item];
        }
    }
    return 1;
}

static int fitComponents(const float *samples, uint32_t sample_count,
                         GSignalResult *result)
{
    uint8_t unknowns = 1U + 2U * result->component_count;
    float system[7][8] = {{0.0f}};
    float basis[7];

    for (uint32_t sample = 0U; sample < sample_count; sample++)
    {
        float time = sample / G_SAMPLE_RATE_HZ;
        basis[0] = 1.0f;
        for (uint8_t component = 0U;
             component < result->component_count; component++)
        {
            float angle = 2.0f * G_PI *
                          result->components[component].frequency_hz * time;
            basis[1U + 2U * component] = cosf(angle);
            basis[2U + 2U * component] = sinf(angle);
        }
        for (uint8_t row = 0U; row < unknowns; row++)
        {
            system[row][unknowns] += basis[row] * samples[sample];
            for (uint8_t column = row; column < unknowns; column++)
                system[row][column] += basis[row] * basis[column];
        }
    }
    for (uint8_t row = 0U; row < unknowns; row++)
        for (uint8_t column = 0U; column < row; column++)
            system[row][column] = system[column][row];
    if (!solveLinearSystem(system, unknowns))
        return 0;

    for (uint8_t component = 0U;
         component < result->component_count; component++)
    {
        float cosine = system[1U + 2U * component][unknowns];
        float sine = system[2U + 2U * component][unknowns];
        result->components[component].amplitude_v =
            sqrtf(cosine * cosine + sine * sine);
        result->components[component].phase_rad = atan2f(cosine, sine);
    }
    return 1;
}

static int pruneWeakComponents(const float *samples, uint32_t sample_count,
                               GSignalResult *result)
{
    if (result->component_count <= 1U)
        return 1;

    uint8_t retained = 1U;
    for (uint8_t index = 1U; index < result->component_count; index++)
    {
        if (result->components[index].amplitude_v <
            G_COMPONENT_REPORT_THRESHOLD_V)
            continue;
        if (retained != index)
            result->components[retained] = result->components[index];
        retained++;
    }
    if (retained == result->component_count)
        return 1;

    result->component_count = retained;
    return fitComponents(samples, sample_count, result);
}

static float reconstructedValue(const GSignalResult *result, float time)
{
    float value = 0.0f;
    for (uint8_t component = 0U;
         component < result->component_count; component++)
    {
        const GSignalComponent *item = &result->components[component];
        value += item->amplitude_v *
                 sinf(2.0f * G_PI * item->frequency_hz * time +
                      item->phase_rad);
    }
    return value;
}

static float findRisingMidpointTime(const GSignalResult *result)
{
    float minimum = 1e30f;
    float maximum = -1e30f;
    for (uint16_t step = 0U; step < G_TIME_WAVE_ALIGNMENT_STEPS; step++)
    {
        float time = step /
                     (G_TIME_WAVE_ALIGNMENT_STEPS * result->fundamental_hz);
        float value = reconstructedValue(result, time);
        if (value < minimum) minimum = value;
        if (value > maximum) maximum = value;
    }

    float midpoint = 0.5f * (minimum + maximum);
    float previous_time = 0.0f;
    float previous = reconstructedValue(result, previous_time);
    for (uint16_t step = 1U; step <= G_TIME_WAVE_ALIGNMENT_STEPS; step++)
    {
        float time = step /
                     (G_TIME_WAVE_ALIGNMENT_STEPS * result->fundamental_hz);
        float current = reconstructedValue(result, time);
        if (previous <= midpoint && current > midpoint)
        {
            float fraction = (midpoint - previous) / (current - previous);
            return previous_time + fraction * (time - previous_time);
        }
        previous_time = time;
        previous = current;
    }
    return 0.0f;
}

void GSignal_RecalculateMetrics(GSignalResult *result)
{
    if (result == NULL || result->component_count == 0U ||
        result->fundamental_hz <= 0.0f)
        return;

    float rms_power = 0.0f;
    for (uint8_t component = 0U;
         component < result->component_count; component++)
    {
        float amplitude = result->components[component].amplitude_v;
        rms_power += amplitude * amplitude * 0.5f;
    }
    result->vrms_v = sqrtf(rms_power);

    float minimum = 1e30f;
    float maximum = -1e30f;
    for (uint32_t point = 0U; point < 4096U; point++)
    {
        float time = point / (4096.0f * result->fundamental_hz);
        float value = reconstructedValue(result, time);
        if (value < minimum) minimum = value;
        if (value > maximum) maximum = value;
    }
    result->vpp_v = maximum - minimum;
}

int GSignal_Analyze(const float *samples, uint32_t sample_count,
                    GSignalMode mode, GSignalResult *result)
{
    if (samples == NULL || result == NULL || sample_count < G_CAPTURE_SIZE ||
        mode >= G_SIGNAL_MODE_COUNT)
        return 0;

    memset(result, 0, sizeof(*result));
    prepareSpectrum(samples, sample_count);
    result->frequency_resolution_hz =
        (G_SAMPLE_RATE_HZ / G_DECIMATION) / G_FFT_SIZE;
    result->component_count = findHarmonicFamily(
        GSignal_ModeMaxFrequency(mode), result->components,
        &result->fundamental_hz);
    if (result->component_count == 0U ||
        !fitComponents(samples, sample_count, result) ||
        !pruneWeakComponents(samples, sample_count, result))
        return 0;

    GSignal_RecalculateMetrics(result);
    result->valid = 1U;
    return 1;
}

void GSignal_BuildTimeWave(const GSignalResult *result, uint8_t periods,
                           uint8_t *output, uint16_t points)
{
    if (result == NULL || output == NULL || points < 2U || !result->valid)
        return;
    if (periods != 3U) periods = 1U;

    float start_time = findRisingMidpointTime(result);
    float minimum = 1e30f;
    float maximum = -1e30f;
    for (uint16_t point = 0U; point < points; point++)
    {
        float time = start_time + periods * point /
                     ((points - 1.0f) * result->fundamental_hz);
        float value = reconstructedValue(result, time);
        if (value < minimum) minimum = value;
        if (value > maximum) maximum = value;
    }
    float span = maximum - minimum;
    if (span <= 0.0f) span = 1.0f;
    for (uint16_t point = 0U; point < points; point++)
    {
        float time = start_time + periods * point /
                     ((points - 1.0f) * result->fundamental_hz);
        float normalized = (reconstructedValue(result, time) - minimum) / span;
        int value = (int)(10.0f + 235.0f * normalized + 0.5f);
        if (value < 0) value = 0;
        if (value > 255) value = 255;
        output[point] = (uint8_t)value;
    }
    output[points - 1U] = output[0];
}

void GSignal_BuildSpectrumWave(const GSignalResult *result, GSignalMode mode,
                               uint8_t *output, uint16_t points)
{
    if (result == NULL || output == NULL || points < 4U)
        return;
    memset(output, G_SPECTRUM_AXIS_BASELINE, points);
    output[0] = G_SPECTRUM_AXIS_TOP;
    output[1] = G_SPECTRUM_AXIS_BASELINE;
    if (!result->valid || result->component_count == 0U)
        return;

    float maximum_amplitude = 0.0f;
    for (uint8_t component = 0U;
         component < result->component_count; component++)
        if (result->components[component].amplitude_v > maximum_amplitude)
            maximum_amplitude = result->components[component].amplitude_v;
    if (maximum_amplitude <= 0.0f)
        return;

    float maximum_frequency = GSignal_ModeMaxFrequency(mode);
    uint16_t last_data_point = points - 2U;
    uint16_t data_width = last_data_point - G_SPECTRUM_DATA_FIRST_POINT;
    for (uint8_t component = 0U;
         component < result->component_count; component++)
    {
        float frequency_hz = result->components[component].frequency_hz;
        if (frequency_hz > maximum_frequency)
            continue;
        uint16_t position = G_SPECTRUM_DATA_FIRST_POINT + (uint16_t)(
            frequency_hz * data_width / maximum_frequency + 0.5f);
        float ratio = result->components[component].amplitude_v /
                      maximum_amplitude;
        output[position] = (uint8_t)(G_SPECTRUM_AXIS_BASELINE +
            (G_SPECTRUM_AXIS_TOP - G_SPECTRUM_AXIS_BASELINE) * ratio + 0.5f);
    }
}
