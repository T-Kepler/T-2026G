/*******************************************************************************
 * @file      my_fft.c
 * @author    左岚
 * @version   V1.1
 * @date      2025-07-18
 * @brief     FFT(快速傅里叶变换)处理与频谱分析功能实现
 * @note      本文件基于ARM官方的CMSIS-DSP库实现了信号的频谱分析。
 * 主要功能包括：FFT模块初始化、相干采样频谱计算、
 * 幅度谱归一化、峰值频率插值估计以及总谐波失真(THD)计算。
 *******************************************************************************/

#include "my_fft.h"
#include "arm_const_structs.h"
#include <math.h>

// ============================= FFT相关全局变量 =============================
// ARM CMSIS-DSP库所需的基4 FFT实例结构体
arm_cfft_radix4_instance_f32 fft_instance;
// FFT输入缓冲区。长度为 FFT_LENGTH * 2，因为需要存储复数（实部、虚部交错）
float fft_input_buffer[FFT_LENGTH * 2];
// FFT幅度谱输出缓冲区
float fft_magnitude[FFT_LENGTH];
static float coarse_window[FFT_COARSE_LENGTH];
static float spectrum_window[FFT_LENGTH];

/**
 * @brief  FFT模块初始化
 * @details 此函数初始化ARM CMSIS-DSP的FFT实例。
 * @param  None
 * @retval None
 */
void fft_init(void)
{
    // 初始化radix-4 FFT实例结构体
    arm_cfft_radix4_init_f32(&fft_instance, FFT_LENGTH, 0, 1);
    for (uint16_t i = 0; i < FFT_COARSE_LENGTH; i++)
    {
        coarse_window[i] = 0.5f * (1.0f - arm_cos_f32(
            2.0f * 3.14159265f * i / (FFT_COARSE_LENGTH - 1)));
    }
    for (uint16_t i = 0; i < FFT_LENGTH; i++)
    {
        spectrum_window[i] = 0.5f * (1.0f - arm_cos_f32(
            2.0f * 3.14159265f * i / (FFT_LENGTH - 1)));
    }
    // 参数说明:
    // &fft_instance:  指向FFT实例的指针
    // FFT_LENGTH:     FFT点数 (例如: 1024)
    // 0:              ifftFlag, 0表示正向FFT, 1表示逆向IFFT
    // 1:              bitReverseFlag, 1表示输出结果按正常顺序排列
}

int fft_find_dominant_frequency(const float *input_data, float sampling_freq,
                                float *frequency_hz, float *magnitude)
{
    float mean = 0.0f;
    float window_sum = 0.0f;
    float max_magnitude = 0.0f;
    uint16_t max_index = 1;

    if (input_data == NULL || frequency_hz == NULL || magnitude == NULL ||
        sampling_freq <= 0.0f)
    {
        return 0;
    }

    for (uint16_t i = 0; i < FFT_COARSE_LENGTH; i++)
        mean += input_data[i];
    mean /= FFT_COARSE_LENGTH;

    memset(fft_input_buffer, 0, FFT_COARSE_LENGTH * 2 * sizeof(float));
    for (uint16_t i = 0; i < FFT_COARSE_LENGTH; i++)
    {
        fft_input_buffer[2 * i] = (input_data[i] - mean) * coarse_window[i];
        window_sum += coarse_window[i];
    }

    arm_cfft_f32(&arm_cfft_sR_f32_len128, fft_input_buffer, 0, 1);
    arm_cmplx_mag_f32(fft_input_buffer, fft_magnitude, FFT_COARSE_LENGTH);

    for (uint16_t i = 1; i < FFT_COARSE_LENGTH / 2; i++)
    {
        if (fft_magnitude[i] > max_magnitude)
        {
            max_magnitude = fft_magnitude[i];
            max_index = i;
        }
    }

    float delta = 0.0f;
    if (max_index > 1 && max_index < FFT_COARSE_LENGTH / 2 - 1)
    {
        float left = fft_magnitude[max_index - 1];
        float center = fft_magnitude[max_index];
        float right = fft_magnitude[max_index + 1];
        float denominator = left - 2.0f * center + right;
        if (fabsf(denominator) >= 1e-10f)
        {
            delta = 0.5f * (left - right) / denominator;
            if (delta > 0.5f) delta = 0.5f;
            if (delta < -0.5f) delta = -0.5f;
        }
    }

    *frequency_hz = (max_index + delta) * sampling_freq / FFT_COARSE_LENGTH;
    *magnitude = max_magnitude * 2.0f / window_sum;
    return 1;
}

/**
 * @brief  计算输入数据的FFT频谱
 * @param  input_data  指向原始采样数据的浮点数组指针
 * @param  data_length 输入数据的实际长度 (最大为FFT_LENGTH)
 * @retval None
 */
#define FFT_MAIN_LOBE_RADIUS 2U
#define FFT_HANN_POWER_BW 1.5f
#define FFT_PHASE_MIN_MAGNITUDE 0.02f

void calculate_fft_spectrum(const float *input_data, uint16_t data_length)
{
    uint16_t i;
    uint16_t actual_length = (data_length > FFT_LENGTH) ? FFT_LENGTH : data_length;
    float mean = 0.0f;
    float window_sum = 0.0f;

    // 1. 清空缓冲区，确保每次计算的初始状态干净
    memset(fft_input_buffer, 0, sizeof(fft_input_buffer));
    memset(fft_magnitude, 0, sizeof(fft_magnitude));

    for (i = 0; i < actual_length; i++) mean += input_data[i];
    if (actual_length > 0U) mean /= actual_length;

    // 2. Hann窗抑制非相干采样泄漏，幅度按窗函数相干增益补偿。
    for (i = 0; i < actual_length; i++)
    {
        float window = spectrum_window[i];
        fft_input_buffer[2*i]     = (input_data[i] - mean) * window;
        fft_input_buffer[2*i + 1] = 0.0f;
        window_sum += window;
    }
    // 如果实际数据长度小于FFT_LENGTH，缓冲区剩余部分已清零，这相当于进行了“零填充(Zero Padding)”。
    // 零填充不提高频谱的物理分辨率，但可以增加频点密度，有助于观察和插值。

    // 3. 执行核心的复数FFT计算
    arm_cfft_radix4_f32(&fft_instance, fft_input_buffer);

    // 4. 计算复数结果的模，得到幅度谱
    arm_cmplx_mag_f32(fft_input_buffer, fft_magnitude, FFT_LENGTH);

    // 5. Hann窗幅度归一化；直流分量保留采样均值。
    fft_magnitude[0] = fabsf(mean);
    for (i = 1; i < FFT_LENGTH / 2; i++)
    {
        fft_magnitude[i] = (window_sum > 0.0f)
            ? fft_magnitude[i] * 2.0f / window_sum : 0.0f;
    }
}

int fft_get_phase(const float *input_data, float sampling_freq,
                  float signal_freq, float *phase_deg)
{
    if (input_data == NULL || phase_deg == NULL || sampling_freq <= 0.0f ||
        signal_freq <= 0.0f || signal_freq >= sampling_freq * 0.5f)
        return 0;

    uint16_t target_bin = (uint16_t)(signal_freq * FFT_LENGTH /
                                     sampling_freq + 0.5f);
    if (target_bin < 1U || target_bin >= FFT_LENGTH / 2U)
        return 0;

    calculate_fft_spectrum(input_data, FFT_LENGTH);
    if (fft_magnitude[target_bin] < FFT_PHASE_MIN_MAGNITUDE)
        return 0;

    *phase_deg = atan2f(fft_input_buffer[2U * target_bin + 1U],
                        fft_input_buffer[2U * target_bin]) *
                 (180.0f / 3.14159265f);
    return 1;
}

static float sumSpectrumBandPower(uint16_t center_bin)
{
    uint16_t first = (center_bin > FFT_MAIN_LOBE_RADIUS)
        ? center_bin - FFT_MAIN_LOBE_RADIUS : 1U;
    uint16_t last = center_bin + FFT_MAIN_LOBE_RADIUS;
    float power = 0.0f;

    if (last >= FFT_LENGTH / 2U) last = FFT_LENGTH / 2U - 1U;
    for (uint16_t i = first; i <= last; i++)
        power += fft_magnitude[i] * fft_magnitude[i];
    return power;
}

/**
 * @brief  通过串口输出FFT频谱分析结果
 * @param  None
 * @retval None
 */
void output_fft_spectrum(float sampling_freq)
{
    uint16_t i;
    float freq_resolution;
    float current_freq;

    freq_resolution = sampling_freq / FFT_LENGTH;

    my_printf(&huart1, "=== FFT Spectrum Analysis ===\r\n");
    my_printf(&huart1, "FFT Frequency Scale: %.0f Hz\r\n", sampling_freq);
    my_printf(&huart1, "Freq Resolution: %.2f Hz\r\n", freq_resolution);
    my_printf(&huart1, "--- Spectrum Data ---\r\n");

    // 输出Hann窗幅度谱，仅跳过直流分量。
    for (i = 1; i < FFT_LENGTH / 2; i++)
    {
        current_freq = i * freq_resolution;
        my_printf(&huart1, "%.1f Hz: %.6f\r\n", current_freq, fft_magnitude[i]);
    }

    my_printf(&huart1, "--- Peak Analysis ---\r\n");

    // 使用相邻频点抛物线插值估计峰值频率。
    float peak_freq = get_precise_peak_frequency(sampling_freq);
    uint16_t max_harmonic = 0U;
    if (peak_freq > 0.0f && sampling_freq * 0.5f > freq_resolution)
    {
        max_harmonic = (uint16_t)((sampling_freq * 0.5f - freq_resolution) / peak_freq);
        if (max_harmonic > 15U)
        {
            max_harmonic = 15U;
        }
    }

    // 获取峰值点的幅度（用于显示）
    float max_magnitude = 0.0f;
    uint16_t max_index = 1U;
    for (i = 1; i < FFT_LENGTH / 2; i++)
    {
        if (fft_magnitude[i] > max_magnitude)
        {
            max_magnitude = fft_magnitude[i];
            max_index = i;
        }
    }
    max_magnitude = sqrtf(sumSpectrumBandPower(max_index) / FFT_HANN_POWER_BW);

    // 计算扩展的失真指标
    float thd = (max_harmonic >= 2U) ? calculate_thd(peak_freq, sampling_freq) : 0.0f;
    float thd_n = calculate_thd_n(peak_freq, sampling_freq);
    float sinad = calculate_sinad(peak_freq, sampling_freq);
    
    my_printf(&huart1, "Peak Freq: %.0f Hz, Magnitude: %.6fv\r\n", peak_freq, max_magnitude);
    if (max_harmonic >= 2U)
    {
        my_printf(&huart1, "THD (2-%u): %.3f%%\r\n", max_harmonic, thd);
    }
    else
    {
        my_printf(&huart1, "THD: N/A (sampling frequency too low)\r\n");
    }
    my_printf(&huart1, "THD+N: %.3f%%\r\n", thd_n);
    my_printf(&huart1, "SINAD: %.1f dB\r\n", sinad);
    my_printf(&huart1, "DC Component: %.6fv\r\n", fft_magnitude[0]);
    my_printf(&huart1, "=== End of Spectrum ===\r\n");
}

/**
 * @brief  使用抛物线插值估计峰值频率
 * @details FFT的频率分辨率有限，此方法通过在最大频点及其相邻点之间拟合一条
 * 抛物线，并找到抛物线的顶点，从而得到比FFT分辨率更高的频率估计值。
 * @param  sampling_freq 采样频率
 * @return (float) 插值后的峰值频率
 */
float get_precise_peak_frequency(float sampling_freq)
{
    uint16_t i;
    float freq_resolution = sampling_freq / FFT_LENGTH;

    // 1. 寻找最大幅度点（谱峰）的索引
    float max_magnitude = 0.0f;
    uint16_t max_index = 1;
    for (i = 1; i < FFT_LENGTH / 2; i++)
    {
        if (fft_magnitude[i] > max_magnitude)
        {
            max_magnitude = fft_magnitude[i];
            max_index = i;
        }
    }

    // 如果峰值点在边界，无法进行三点插值，直接返回粗略频率
    if (max_index <= 1 || max_index >= (FFT_LENGTH / 2 - 1))
    {
        return max_index * freq_resolution;
    }

    // 2. 三点抛物线插值
    float y1 = fft_magnitude[max_index - 1]; // 左边点
    float y2 = fft_magnitude[max_index];     // 中间峰值点
    float y3 = fft_magnitude[max_index + 1]; // 右边点

    // 计算抛物线顶点相对于中心点(max_index)的偏移量 delta
    float delta = 0.5f * (y1 - y3) / (y1 - 2.0f * y2 + y3);

    // 检查分母是否过小，防止除以零
    if (fabsf(y1 - 2.0f * y2 + y3) < 1e-10f)
    {
        delta = 0.0f;
    }
    
    // 限制偏移量范围，确保结果合理
    if (delta > 0.5f)  delta = 0.5f;
    if (delta < -0.5f) delta = -0.5f;

    // 3. 插值估计峰值频率
    float precise_freq = (max_index + delta) * freq_resolution;

    return precise_freq;
}

/**
 * @brief  计算总谐波失真 (THD)
 * @details THD = sqrt(∑(谐波功率)) / sqrt(基波功率) × 100%
 *          统计2至15次、且低于奈奎斯特频率的Hann主瓣功率。
 * @param  fundamental_freq 基波频率
 * @param  sampling_freq 采样频率  
 * @return (float) THD值（以百分比形式返回）
 */
float calculate_thd(float fundamental_freq, float sampling_freq)
{
    float freq_resolution = sampling_freq / FFT_LENGTH;
    float fundamental_power;
    float harmonic_power = 0.0f;

    uint16_t fundamental_bin = (uint16_t)(fundamental_freq / freq_resolution + 0.5f);
    if (fundamental_bin < 1 || fundamental_bin >= FFT_LENGTH / 2)
    {
        return 0.0f;
    }

    fundamental_power = sumSpectrumBandPower(fundamental_bin);

    // 累计每次谐波的完整Hann主瓣，避免频率偏差造成单频点漏算。
    for (uint8_t harmonic = 2; harmonic <= 15; harmonic++)
    {
        uint16_t harmonic_bin = (uint16_t)(harmonic * fundamental_freq / freq_resolution + 0.5f);
        if (harmonic_bin >= FFT_LENGTH / 2)
        {
            break;
        }

        harmonic_power += sumSpectrumBandPower(harmonic_bin);
    }

    if (fundamental_power > 1e-20f)
    {
        return sqrtf(harmonic_power / fundamental_power) * 100.0f;
    }

    return 0.0f;
}

/**
 * @brief  计算THD+N（总谐波失真加噪声）
 * @details THD+N = sqrt(∑(总功率 - 基波功率)) / sqrt(基波功率) × 100%
 *          包含了谐波失真和噪声的综合指标，更全面地评估信号质量
 * @param  fundamental_freq 基波频率
 * @param  sampling_freq 采样频率
 * @return (float) THD+N值（以百分比形式返回）
 */
float calculate_thd_n(float fundamental_freq, float sampling_freq)
{
    float freq_resolution = sampling_freq / FFT_LENGTH;
    float fundamental_power;
    float total_power = 0.0f;

    uint16_t fundamental_bin = (uint16_t)(fundamental_freq / freq_resolution + 0.5f);
    if (fundamental_bin < 1 || fundamental_bin >= FFT_LENGTH / 2)
    {
        return 0.0f;
    }

    fundamental_power = sumSpectrumBandPower(fundamental_bin);

    // 总功率排除直流，再扣除基波完整Hann主瓣。
    for (uint16_t i = 1; i < FFT_LENGTH / 2; i++)
    {
        total_power += fft_magnitude[i] * fft_magnitude[i];
    }

    float distortion_noise_power = total_power - fundamental_power;
    if (fundamental_power > 0.0f && distortion_noise_power >= 0.0f)
    {
        return sqrtf(distortion_noise_power / fundamental_power) * 100.0f;
    }

    return 0.0f;
}

/**
 * @brief  计算信纳比(SINAD - Signal to Noise and Distortion Ratio)
 * @details SINAD = 基波功率 / (噪声功率 + 失真功率) (dB)
 *          SINAD是THD+N的倒数，以dB形式表示信号质量
 * @param  fundamental_freq 基波频率
 * @param  sampling_freq 采样频率
 * @return (float) SINAD值（以dB形式返回）
 */
float calculate_sinad(float fundamental_freq, float sampling_freq)
{
    float thd_n_percent = calculate_thd_n(fundamental_freq, sampling_freq);
    
    if (thd_n_percent > 0.0f)
    {
        // SINAD(dB) = -20*log10(THD+N/100)
        float sinad_db = -20.0f * log10f(thd_n_percent / 100.0f);
        return sinad_db;
    }
    
    return 0.0f;
}
