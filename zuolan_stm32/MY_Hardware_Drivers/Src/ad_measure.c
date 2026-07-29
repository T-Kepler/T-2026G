/*******************************************************************************
 * @file      ad_measure.c
 * @author    左岚
 * @brief     ADC采样率、FIFO采集、幅度、自动测频和双路相位测量
 *******************************************************************************/

#include "ad_measure.h"
#include "my_fft.h"
#include "stm32f4xx_hal.h"

// --- 宏定义 ---

// AD9226模块为12位偏移二进制，输入量程为-5V~+5V。
#define ADC_SCALE 2048.0f
#define ADC_INPUT_HALF_RANGE 5.0f
#define ADC_MAX_SAMPLE_RATE 65000000U
#define ADC_CAPTURE_TIMEOUT_MS 100U
#define ADC_LOW_DIRECT_RATIO 128.0f
#define ADC_HIGH_DIRECT_RATIO 64.0f
#define ADC_DIRECT_RATIO_SWITCH 100000.0f
#define ADC_FREQ_MIN_RANGE_COUNTS 12U
#define ADC_FREQ_MIN_CROSSINGS 4
#define ADC_FREQ_MIN_PEAK_VOLTS 0.02f
#define ADC_FREQ_PEAK_RATIO 0.8f
#define ADC_FREQ_FINE_HIGH_RATIO 8.0f
#define ADC_FREQ_FINE_LOW_RATIO 24.0f
#define ADC_FIFO_READ_GAP_NOPS 16U




// --- 全局变量 ---

// 用于存储ADC通道1和通道2采样数据中的最大值和最小值（原始ADC值）
u16 vol_maxnum1, vol_minnum1, vol_maxnum2, vol_minnum2;

// 用于存储从硬件FIFO中读取的原始采样数据 (16位无符号整数)
// AD_FIFO_SIZE 是在 "ad_measure.h" 中定义的FIFO缓冲区大小
u16 fifo_data1[AD_FIFO_SIZE], fifo_data2[AD_FIFO_SIZE];

// 用于存储转换为实际电压值的浮点数结果
float fifo_data1_f[AD_FIFO_SIZE], fifo_data2_f[AD_FIFO_SIZE];

// 原始采样极值计算得到的峰峰值。
float vol_amp1, vol_amp2;

// FPGA实际采样率，以及FFT频率轴使用的实际/等效采样率。
static float adc_sampling_freq[2] = {0.0f, 0.0f};
static float adc_fft_sampling_freq[2] = {0.0f, 0.0f};


// --- 函数实现 ---

static void waitForFifoAdvance(void)
{
    for (volatile uint32_t remaining = ADC_FIFO_READ_GAP_NOPS;
         remaining > 0U; remaining--)
        __NOP();
}

static float adcCodeToVoltage(u16 raw_code)
{
    return (raw_code - ADC_SCALE) * ADC_INPUT_HALF_RANGE / ADC_SCALE;
}

/**
 * @brief  在给定的无符号16位整型数组中查找最大值和最小值。
 * @param  numbers: 指向待搜索数组的指针。
 * @param  size:    数组的元素个数。
 * @param  max:     指向用于存储最大值的变量的指针。
 * @param  min:     指向用于存储最小值的变量的指针。
 * @retval None
 */
void findMinMax(u16 *numbers, int size, u16 *max, u16 *min)
{
    // 初始化最大值和最小值为数组的第一个元素
    *max = numbers[0];
    *min = numbers[0];

    // 从第二个元素开始遍历整个数组
    for (int i = 1; i < size; i++)
    {
        // 如果当前元素大于当前最大值，则更新最大值
        if (numbers[i] > *max)
        {
            *max = numbers[i];
        }
        // 如果当前元素小于当前最小值，则更新最小值
        if (numbers[i] < *min)
        {
            *min = numbers[i];
        }
    }
}

/**
 * @brief  设置指定ADC通道的采样频率。
 * @param  freq:    频率值 (单位: Hz)。其具体含义由 'mode' 参数决定。
 * @param  channel: 目标ADC通道号 (1 或 2)。
 * @param  mode:    频率模式，决定如何解释 'freq' 参数。
 * - FREQ_MODE_SIGNAL: 'freq' 被视为输入信号的频率。
 * - FREQ_MODE_SAMPLING: 'freq' 被视为要直接设置的采样频率。
 * @retval None
 * @note   此函数会根据所选模式计算出最终的采样率(fs)，并生成一个频率控制字(M)，
 * 然后将该控制字写入对应的硬件寄存器来配置FPGA或相关外设。
 */
static void setSamplingFrequency(float freq, int channel, AdcFrequencyMode mode)
{
    if (freq <= 0.0f || (channel != 1 && channel != 2))
        return;

    unsigned int fs = 0; // 最终要设置的采样频率 (Sampling Frequency)

    if (mode == FREQ_MODE_SAMPLING)
    {
        // 直接指定采样频率模式：直接使用传入的值作为采样率
        fs = (unsigned int)freq;
    }
    else
    {
        // 正式幅度/THD统一直接采样；达到上限时改选1024点整数周期采样率。
        float ratio = (freq <= ADC_DIRECT_RATIO_SWITCH)
            ? ADC_LOW_DIRECT_RATIO : ADC_HIGH_DIRECT_RATIO;
        float target_sampling_freq = freq * ratio;
        if (target_sampling_freq > ADC_MAX_SAMPLE_RATE)
        {
            float cycles_exact = freq * AD_FIFO_SIZE / ADC_MAX_SAMPLE_RATE;
            unsigned int cycles = (unsigned int)cycles_exact;
            if ((float)cycles < cycles_exact) cycles++;
            target_sampling_freq = freq * AD_FIFO_SIZE / cycles;
        }
        fs = (unsigned int)(target_sampling_freq + 0.5f);
    }

    if (fs > ADC_MAX_SAMPLE_RATE)
    {
        fs = ADC_MAX_SAMPLE_RATE;
    }

    // 32位NCO频率字：M = fs * 2^32 / 150MHz。
    unsigned int M = AD_FREQ_CONSTANT * fs / FPGA_BASE_CLK;

    // 通过FMC写入目标采样NCO。
    if (channel == 1)
    {
        AD_FREQ_SET(1);
        HAL_Delay(1);
        AD1_FS_H = M >> 16;  // 写入频率控制字的高16位
        HAL_Delay(1);
        AD1_FS_L = M & 0xFFFF; // 写入频率控制字的低16位
    }
    else
    {
        AD_FREQ_SET(2);
        HAL_Delay(1);
        AD2_FS_H = M >> 16;  // 写入频率控制字的高16位
        HAL_Delay(1);
        AD2_FS_L = M & 0xFFFF; // 写入频率控制字的低16位
    }

    float actual_fs = (float)((double)M * FPGA_BASE_CLK /
                              4294967296.0);
    adc_sampling_freq[channel - 1] = actual_fs;
    adc_fft_sampling_freq[channel - 1] = actual_fs;
}

/**
 * @brief 用带迟滞的上升沿过零点估算频率。
 * @note 最少需要4个同向过零点，即覆盖3个完整周期。
 */
static float estimateFrequency(const u16 *samples, int size, float sampling_freq)
{
    u16 max_value = samples[0];
    u16 min_value = samples[0];
    for (int i = 1; i < size; i++)
    {
        if (samples[i] > max_value) max_value = samples[i];
        if (samples[i] < min_value) min_value = samples[i];
    }

    u16 range = max_value - min_value;
    if (range < ADC_FREQ_MIN_RANGE_COUNTS)
    {
        return 0.0f;
    }

    float midpoint = min_value + range * 0.5f;
    float arm_level = min_value + range * 0.4f;
    float first_crossing = 0.0f;
    float last_crossing = 0.0f;
    int crossing_count = 0;
    int armed = samples[0] <= arm_level;

    for (int i = 1; i < size; i++)
    {
        float previous = samples[i - 1];
        float current = samples[i];

        if (current <= arm_level)
        {
            armed = 1;
        }
        if (armed && previous < midpoint && current >= midpoint && current > previous)
        {
            float crossing = (i - 1) + (midpoint - previous) / (current - previous);
            if (crossing_count == 0) first_crossing = crossing;
            last_crossing = crossing;
            crossing_count++;
            armed = 0;
        }
    }

    if (crossing_count < ADC_FREQ_MIN_CROSSINGS || last_crossing <= first_crossing)
    {
        return 0.0f;
    }

    float frequency = sampling_freq * (crossing_count - 1) /
                      (last_crossing - first_crossing);
    return (frequency <= sampling_freq * 0.25f) ? frequency : 0.0f;
}

static void setShortCaptureMode(int channel, int enable)
{
    u16 mask = (channel == 1) ? AD1_FIFO_SHORT : AD2_FIFO_SHORT;
    CTRL_DATA = enable ? (CTRL_DATA | mask) : (CTRL_DATA & (~mask));
}

static void resetFifo(int channel)
{
    u16 mask = (channel == 1) ? AD1_FIFO_RESET : AD2_FIFO_RESET;

    AD_FIFO_WRITE_DISABLE(channel);
    AD_FIFO_READ_DISABLE(channel);
    CTRL_DATA = CTRL_DATA | mask;
    HAL_Delay(1);
    CTRL_DATA = CTRL_DATA & (~mask);
    HAL_Delay(1);
}

static void resetFifoPair(void)
{
    CTRL_DATA = CTRL_DATA & (~(AD1_FIFO_WR | AD2_FIFO_WR |
                               AD1_FIFO_RD | AD2_FIFO_RD));
    CTRL_DATA = CTRL_DATA | AD1_FIFO_RESET | AD2_FIFO_RESET;
    HAL_Delay(1);
    CTRL_DATA = CTRL_DATA & (~(AD1_FIFO_RESET | AD2_FIFO_RESET));
    HAL_Delay(1);
}

float get_adc_sampling_frequency(int channel)
{
    if (channel < 1 || channel > 2)
    {
        return 0.0f;
    }
    return adc_sampling_freq[channel - 1];
}

float get_adc_fft_sampling_frequency(int channel)
{
    if (channel < 1 || channel > 2)
    {
        return 0.0f;
    }
    return adc_fft_sampling_freq[channel - 1];
}

float ad_estimate_captured_frequency(int channel)
{
    if (channel == 1)
        return estimateFrequency(fifo_data1, AD_FIFO_SIZE, adc_sampling_freq[0]);
    if (channel == 2)
        return estimateFrequency(fifo_data2, AD_FIFO_SIZE, adc_sampling_freq[1]);
    return 0.0f;
}

/**
 * @brief  从指定通道的硬件FIFO中读取数据到内存缓冲区。
 * @param  channel:      目标ADC通道号 (1 或 2)。
 * @param  fifo_data:    用于存储原始ADC采样数据的数组指针。
 * @param  fifo_data_f:  用于存储转换为浮点电压值的数组指针。
 * @retval 1表示全部样本有效，0表示检测到非12位的无效FMC读数
 */
static int readFIFOData(int channel, u16 *fifo_data, float *fifo_data_f, int size)
{
    int valid = 1;

    // 读使能同时推进FIFO读指针。
    if (channel == 1)
        AD_FIFO_READ_ENABLE(1);
    else
        AD_FIFO_READ_ENABLE(2);

    HAL_Delay(1);

    for (int i = 0; i < size; i++)
    {
        u16 raw_data = (channel == 1) ? AD1_DATA_SHOW : AD2_DATA_SHOW;
        waitForFifoAdvance();

        // FPGA有效ADC数据固定为12位；高4位非零表示本次FMC读回无效。
        if ((raw_data & 0xF000U) != 0U)
        {
            valid = 0;
            raw_data = (i > 0) ? fifo_data[i - 1] : 0x0800U;
        }
        fifo_data[i] = raw_data;

        fifo_data_f[i] = adcCodeToVoltage(fifo_data[i]);
    }

    // 关闭读使能，避免后续FMC访问继续推进FIFO。
    if (channel == 1)
        AD_FIFO_READ_DISABLE(1);
    else
        AD_FIFO_READ_DISABLE(2);

    return valid;
}

static int captureAdSamples(int channel, float sampling_freq, int sample_count)
{
    u16 *raw_data = (channel == 1) ? fifo_data1 : fifo_data2;
    float *voltage_data = (channel == 1) ? fifo_data1_f : fifo_data2_f;

    setSamplingFrequency(sampling_freq, channel, FREQ_MODE_SAMPLING);
    setShortCaptureMode(channel, sample_count == AD_SHORT_SIZE);
    resetFifo(channel);
    AD_FIFO_WRITE_ENABLE(channel);
    HAL_Delay(1);

    if (channel == 1)
        while (AD1_FULL_FLAG != 0x0001) ;
    else
        while (AD2_FULL_FLAG != 0x0001) ;

    AD_FIFO_WRITE_DISABLE(channel);
    int valid = readFIFOData(channel, raw_data, voltage_data, sample_count);
    setShortCaptureMode(channel, 0);
    return valid;
}

static int failFrequencyDetection(int channel, float *frequency_hz)
{
    if (frequency_hz != NULL)
        *frequency_hz = 0.0f;
    if (channel == 1 || channel == 2)
    {
        adc_sampling_freq[channel - 1] = 0.0f;
        adc_fft_sampling_freq[channel - 1] = 0.0f;
        if (channel == 1) vol_amp1 = 0.0f;
        else vol_amp2 = 0.0f;
    }
    return 0;
}

/**
 * @brief  配置一个或两个通道，等待采集完成并更新有效通道的Vpp。
 * @param  freq1: AD1 的频率值 (单位: Hz)。如果为0，则不采集此通道。
 * @param  mode1: AD1 的频率模式。
 * @param  freq2: AD2 的频率值 (单位: Hz)。如果为0，则不采集此通道。
 * @param  mode2: AD2 的频率模式。
 * @retval None
 */
void vpp_adc_parallel(float freq1, AdcFrequencyMode mode1, float freq2, AdcFrequencyMode mode2)
{
    if (freq1 > 0)
    {
        setShortCaptureMode(1, 0);
        setSamplingFrequency(freq1, 1, mode1);
        resetFifo(1);
        AD_FIFO_WRITE_ENABLE(1);
    }

    if (freq2 > 0)
    {
        setShortCaptureMode(2, 0);
        setSamplingFrequency(freq2, 2, mode2);
        resetFifo(2);
        AD_FIFO_WRITE_ENABLE(2);
    }

    HAL_Delay(1);

    if (freq1 > 0)
    {
        while (AD1_FULL_FLAG != 0x0001)
            ;
        AD_FIFO_WRITE_DISABLE(1);
    }

    if (freq2 > 0)
    {
        while (AD2_FULL_FLAG != 0x0001)
            ;
        AD_FIFO_WRITE_DISABLE(2);
    }

    if (freq1 > 0)
    {
        if (readFIFOData(1, fifo_data1, fifo_data1_f, AD_FIFO_SIZE))
        {
            findMinMax(fifo_data1, AD_FIFO_SIZE, &vol_maxnum1, &vol_minnum1);
            vol_amp1 = (vol_maxnum1 - vol_minnum1) *
                       ADC_INPUT_HALF_RANGE / ADC_SCALE;
        }
    }

    if (freq2 > 0)
    {
        if (readFIFOData(2, fifo_data2, fifo_data2_f, AD_FIFO_SIZE))
        {
            findMinMax(fifo_data2, AD_FIFO_SIZE, &vol_maxnum2, &vol_minnum2);
            vol_amp2 = (vol_maxnum2 - vol_minnum2) *
                       ADC_INPUT_HALF_RANGE / ADC_SCALE;
        }
    }
}

int ad_measure_phase_difference(float signal_freq, float calibration_ns,
                                float *phase_difference_deg)
{
    float phase1;
    float phase2;

    if (signal_freq <= 0.0f || phase_difference_deg == NULL)
        return 0;

    vol_amp1 = 0.0f;
    vol_amp2 = 0.0f;
    setShortCaptureMode(1, 0);
    setShortCaptureMode(2, 0);
    setSamplingFrequency(signal_freq, 1, FREQ_MODE_SIGNAL);
    setSamplingFrequency(signal_freq, 2, FREQ_MODE_SIGNAL);
    CTRL_DATA = CTRL_DATA | AD_SYNC_MODE;
    resetFifoPair();
    AD_FIFO_WRITE_ENABLE(3);
    HAL_Delay(1);

    while (AD1_FULL_FLAG != 0x0001 || AD2_FULL_FLAG != 0x0001)
        ;

    CTRL_DATA = CTRL_DATA & (~(AD1_FIFO_WR | AD2_FIFO_WR));
    int valid1 = readFIFOData(1, fifo_data1, fifo_data1_f, AD_FIFO_SIZE);
    int valid2 = readFIFOData(2, fifo_data2, fifo_data2_f, AD_FIFO_SIZE);
    CTRL_DATA = CTRL_DATA & (~AD_SYNC_MODE);
    if (!valid1 || !valid2)
        return 0;

    findMinMax(fifo_data1, AD_FIFO_SIZE, &vol_maxnum1, &vol_minnum1);
    findMinMax(fifo_data2, AD_FIFO_SIZE, &vol_maxnum2, &vol_minnum2);
    vol_amp1 = (vol_maxnum1 - vol_minnum1) *
               ADC_INPUT_HALF_RANGE / ADC_SCALE;
    vol_amp2 = (vol_maxnum2 - vol_minnum2) *
               ADC_INPUT_HALF_RANGE / ADC_SCALE;

    if (!fft_get_phase(fifo_data1_f, adc_fft_sampling_freq[0], signal_freq,
                       &phase1))
        return 0;
    if (!fft_get_phase(fifo_data2_f, adc_fft_sampling_freq[0], signal_freq,
                       &phase2))
        return 0;

    // 两个NCO同步装载0°/180°；同序号AD2样本比AD1晚半个实际采样周期。
    float sampling_skew_deg = signal_freq * 180.0f / adc_fft_sampling_freq[0];
    float calibration_deg = calibration_ns * 1e-9f * signal_freq * 360.0f;
    *phase_difference_deg = phase2 - phase1 - sampling_skew_deg - calibration_deg;
    while (*phase_difference_deg > 180.0f) *phase_difference_deg -= 360.0f;
    while (*phase_difference_deg <= -180.0f) *phase_difference_deg += 360.0f;
    return 1;
}

int ad_detect_frequency(int channel, float *frequency_hz)
{
    static const float coarse_sample_rates[] = {
        65000000.0f,
        65000000.0f / AD_SHORT_SIZE,
        65000000.0f / AD_SHORT_SIZE / AD_SHORT_SIZE
    };
    float candidate_frequency[3];
    float candidate_magnitude[3];
    const float *samples;

    if ((channel != 1 && channel != 2) || frequency_hz == NULL)
    {
        return 0;
    }

    samples = (channel == 1) ? fifo_data1_f : fifo_data2_f;
    for (int i = 0; i < 3; i++)
    {
        if (!captureAdSamples(channel, coarse_sample_rates[i], AD_SHORT_SIZE) ||
            !fft_find_dominant_frequency(samples, coarse_sample_rates[i],
                                         &candidate_frequency[i],
                                         &candidate_magnitude[i]))
        {
            return failFrequencyDetection(channel, frequency_hz);
        }
    }

    float max_magnitude = candidate_magnitude[0];
    for (int i = 1; i < 3; i++)
        if (candidate_magnitude[i] > max_magnitude)
            max_magnitude = candidate_magnitude[i];

    if (max_magnitude < ADC_FREQ_MIN_PEAK_VOLTS)
    {
        return failFrequencyDetection(channel, frequency_hz);
    }

    // 多档幅度接近时优先高采样率，避免选择低采样率产生的混叠频率。
    float coarse_frequency = candidate_frequency[2];
    for (int i = 0; i < 3; i++)
    {
        if (candidate_magnitude[i] >= max_magnitude * ADC_FREQ_PEAK_RATIO)
        {
            coarse_frequency = candidate_frequency[i];
            break;
        }
    }

    float fine_ratio = (coarse_frequency > 60000.0f)
        ? ADC_FREQ_FINE_HIGH_RATIO : ADC_FREQ_FINE_LOW_RATIO;
    float precise_sample_rate = coarse_frequency * fine_ratio;
    if (precise_sample_rate > ADC_MAX_SAMPLE_RATE)
        precise_sample_rate = ADC_MAX_SAMPLE_RATE;

    if (!captureAdSamples(channel, precise_sample_rate, AD_SHORT_SIZE))
    {
        return failFrequencyDetection(channel, frequency_hz);
    }

    const u16 *raw_samples = (channel == 1) ? fifo_data1 : fifo_data2;
    *frequency_hz = estimateFrequency(raw_samples, AD_SHORT_SIZE,
                                      get_adc_sampling_frequency(channel));
    if (*frequency_hz <= 0.0f)
    {
        precise_sample_rate *= 0.5f;
        if (!captureAdSamples(channel, precise_sample_rate, AD_SHORT_SIZE))
            return failFrequencyDetection(channel, frequency_hz);
        *frequency_hz = estimateFrequency(raw_samples, AD_SHORT_SIZE,
                                          get_adc_sampling_frequency(channel));
    }

    return (*frequency_hz > 0.0f)
        ? 1 : failFrequencyDetection(channel, frequency_hz);
}

int ad_capture_g_frame(float sampling_freq_hz, float *voltage_data,
                       u16 *raw_data, u16 *raw_min, u16 *raw_max)
{
    if (voltage_data == NULL || sampling_freq_hz <= 0.0f)
        return 0;

    setSamplingFrequency(sampling_freq_hz, 1, FREQ_MODE_SAMPLING);
    setShortCaptureMode(1, 0);
    G_CAPTURE_CTRL = G_CAPTURE_LONG_AD1;
    resetFifo(1);
    AD_FIFO_WRITE_ENABLE(1);

    uint32_t started_at = HAL_GetTick();
    while (AD1_FULL_FLAG != 0x0001U)
    {
        if (HAL_GetTick() - started_at > ADC_CAPTURE_TIMEOUT_MS)
        {
            AD_FIFO_WRITE_DISABLE(1);
            G_CAPTURE_CTRL = 0U;
            return 0;
        }
    }

    AD_FIFO_WRITE_DISABLE(1);
    AD_FIFO_READ_ENABLE(1);
    HAL_Delay(1);

    u16 minimum = 0x0FFFU;
    u16 maximum = 0U;
    int valid = 1;
    for (uint32_t i = 0; i < G_CAPTURE_SIZE; i++)
    {
        u16 raw = AD1_DATA_SHOW;
        waitForFifoAdvance();
        if ((raw & 0xF000U) != 0U)
        {
            valid = 0;
            raw = 0x0800U;
        }
        if (raw < minimum) minimum = raw;
        if (raw > maximum) maximum = raw;
        if (raw_data != NULL) raw_data[i] = raw;
        voltage_data[i] = adcCodeToVoltage(raw);
    }

    AD_FIFO_READ_DISABLE(1);
    G_CAPTURE_CTRL = 0U;
    if (raw_min != NULL) *raw_min = minimum;
    if (raw_max != NULL) *raw_max = maximum;
    return valid;
}
