/**
 * @file   da_output.c
 * @brief   双通道DA波形发生器驱动程序
 * @details  本文件负责实现对FPGA控制的双通道DA输出模块的软件接口。
 * 它通过管理一个内存中的配置结构体（作为硬件寄存器的“影子”），
 * 提供了设置波形参数（频率、幅度、相位等）的API，并能将这些
 * 配置最终应用到硬件，从而驱动DA模块产生指定波形。
 * @author  左岚
 * @date   2025-07-20
 * @version  1.1
 */
#include "da_output.h"
#include "math.h"

/**
 * @brief DA通道配置参数的内存缓存
 * @details 这是一个模块内的全局数组，用作硬件寄存器配置的软件“影子”或缓存。
 * 所有对DA参数的修改都先发生在这里，之后再通过 DA_Apply_Settings()
 * 函数统一写入硬件。
 * @note   请确保在头文件 da_output.h 中也从 DA_Channel_t 结构体里移除了 step 成员。
 */
DA_Channel_t da_channels[NUM_DA_CHANNELS] = {0};

#define DA_MAX_AMPLITUDE_MV 3080U

static uint32_t DA_FrequencyToWord(float frequency_hz)
{
    if (frequency_hz <= 0.0f)
        return 0U;

    double word = ((double)frequency_hz * DA_FREQ_CONSTANT) / FPGA_BASE_CLK;
    if (word >= 4294967295.0)
        return 0xFFFFFFFFU;

    return (uint32_t)(word + 0.5);
}

static uint16_t DA_PhaseToPoints(uint16_t phase_degree)
{
    uint16_t normalized_angle = phase_degree % 360U;
    uint16_t phase_points = (uint16_t)roundf((float)normalized_angle * DA_PHASE_POINTS / 360.0f);

    return phase_points & 0x7FU;
}

static uint16_t DA_ClampAmplitude(uint16_t amplitude_mv)
{
    if (amplitude_mv > DA_MAX_AMPLITUDE_MV)
        return DA_MAX_AMPLITUDE_MV;

    return amplitude_mv;
}

// 128点、14位偏移二进制波形表；S1切换时由STM32写入FPGA双口RAM。
static const uint16_t da_wave_sine[DA_ROM_SIZE] = {
    8192, 8593, 8994, 9393, 9790, 10182, 10569, 10951,
    11326, 11694, 12053, 12403, 12742, 13071, 13388, 13693,
    13984, 14261, 14524, 14771, 15002, 15218, 15416, 15597,
    15759, 15904, 16030, 16138, 16226, 16294, 16344, 16373,
    16383, 16373, 16344, 16294, 16226, 16138, 16030, 15904,
    15759, 15597, 15416, 15218, 15002, 14771, 14524, 14261,
    13984, 13693, 13388, 13071, 12742, 12403, 12053, 11694,
    11326, 10951, 10569, 10182, 9790, 9393, 8994, 8593,
    8192, 7790, 7389, 6990, 6593, 6201, 5814, 5432,
    5057, 4689, 4330, 3980, 3641, 3312, 2995, 2690,
    2399, 2122, 1859, 1612, 1381, 1165, 967, 786,
    624, 479, 353, 245, 157, 89, 39, 10,
    0, 10, 39, 89, 157, 245, 353, 479,
    624, 786, 967, 1165, 1381, 1612, 1859, 2122,
    2399, 2690, 2995, 3312, 3641, 3980, 4330, 4689,
    5057, 5432, 5814, 6201, 6593, 6990, 7389, 7790
};

static const uint16_t da_wave_square[DA_ROM_SIZE] = {
    16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383,
    16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383,
    16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383,
    16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383,
    16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383,
    16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383,
    16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383,
    16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

static const uint16_t da_wave_triangle[DA_ROM_SIZE] = {
    0, 260, 520, 780, 1040, 1300, 1560, 1820,
    2080, 2340, 2600, 2861, 3121, 3381, 3641, 3901,
    4161, 4421, 4681, 4941, 5201, 5461, 5721, 5981,
    6241, 6501, 6761, 7021, 7281, 7541, 7801, 8061,
    8322, 8582, 8842, 9102, 9362, 9622, 9882, 10142,
    10402, 10662, 10922, 11182, 11442, 11702, 11962, 12222,
    12482, 12742, 13002, 13262, 13522, 13783, 14043, 14303,
    14563, 14823, 15083, 15343, 15603, 15863, 16123, 16383,
    16383, 16123, 15863, 15603, 15343, 15083, 14823, 14563,
    14303, 14043, 13783, 13522, 13262, 13002, 12742, 12482,
    12222, 11962, 11702, 11442, 11182, 10922, 10662, 10402,
    10142, 9882, 9622, 9362, 9102, 8842, 8582, 8322,
    8061, 7801, 7541, 7281, 7021, 6761, 6501, 6241,
    5981, 5721, 5461, 5201, 4941, 4681, 4421, 4161,
    3901, 3641, 3381, 3121, 2861, 2600, 2340, 2080,
    1820, 1560, 1300, 1040, 780, 520, 260, 0
};

// 偏差测试波：二次/五次谐波+直流偏置；范围1276~13445，均值约8400。
static const uint16_t da_wave_deviation[DA_ROM_SIZE] = {
    9134, 9704, 10265, 10805, 11312, 11777, 12190, 12544,
    12837, 13067, 13234, 13343, 13400, 13413, 13391, 13345,
    13285, 13221, 13161, 13114, 13085, 13077, 13091, 13126,
    13177, 13240, 13306, 13368, 13418, 13445, 13444, 13406,
    13328, 13206, 13040, 12832, 12587, 12311, 12013, 11702,
    11389, 11085, 10802, 10547, 10330, 10157, 10031, 9954,
    9923, 9936, 9985, 10063, 10159, 10262, 10362, 10448,
    10509, 10537, 10525, 10469, 10367, 10220, 10031, 9804,
    9548, 9270, 8980, 8687, 8401, 8130, 7880, 7657,
    7463, 7299, 7163, 7051, 6955, 6870, 6786, 6694,
    6584, 6449, 6282, 6076, 5830, 5543, 5215, 4853,
    4463, 4053, 3635, 3219, 2818, 2445, 2109, 1822,
    1590, 1421, 1316, 1276, 1300, 1383, 1518, 1697,
    1910, 2148, 2401, 2659, 2914, 3160, 3392, 3608,
    3808, 3994, 4172, 4346, 4526, 4718, 4931, 5173,
    5452, 5771, 6134, 6544, 6997, 7490, 8016, 8568
};

/**
 * @brief 初始化DA模块
 * @details
 * 此函数在系统启动流程中调用，用于为所有DA通道建立一个已知的、
 * 安全的初始状态。它会设置一组默认波形参数，并立即将其应用到硬件，
 * 确保设备上电后即有稳定的默认波形输出。
 */
void DA_Init(void)
{
    // 为通道0设置默认参数: 20kHz, 幅度1000, 相位90°, STM32波形RAM
    DA_SetConfig(0, 20000.0f, 1000, 90, WAVE_CUSTOM);

    // 为通道1设置默认参数: 20kHz, 幅度1000, 相位0°, STM32波形RAM
    DA_SetConfig(1, 20000.0f, 1000, 0, WAVE_CUSTOM);

    // 上电先上传正弦表，再启动双路输出。
    DA_LoadPresetWaveform(DA_PRESET_SINE);
}

void DA_LoadPresetWaveform(DA_PresetWaveform_t wave)
{
    const uint16_t *samples;

    switch (wave)
    {
    case DA_PRESET_SINE:
        samples = da_wave_sine;
        break;
    case DA_PRESET_SQUARE:
        samples = da_wave_square;
        break;
    case DA_PRESET_TRIANGLE:
        samples = da_wave_triangle;
        break;
    case DA_PRESET_DEVIATION:
        samples = da_wave_deviation;
        break;
    default:
        return;
    }

    DA_FPGA_STOP();
    for (uint16_t i = 0; i < DA_ROM_SIZE; i++)
        DA_WAVE_RAM(i) = samples[i];

    da_channels[0].waveform = WAVE_CUSTOM;
    da_channels[1].waveform = WAVE_CUSTOM;
    DA_Apply_Settings();
}

// =================================================================================
//   单个参数的 Get / Set 函数
// =================================================================================
// 这些函数提供了对单个参数的原子化读写能力，仅操作内存中的配置缓存。
// =================================================================================

/**
 * @brief 设置指定通道的频率 (仅更新内存配置)
 * @param channel_index: 目标通道索引 (0 或 1)
 * @param freq: 频率值，单位 Hz
 */
void DA_SetFREQ(uint8_t channel_index, float freq)
{
    // 参数校验：防止数组越界访问
    if (channel_index >= NUM_DA_CHANNELS)
        return;
    if (freq < 0.0f || freq > DA_OUTPUT_MAX_FREQ)
        return;
    // 更新内存中的配置值
    da_channels[channel_index].frequency = freq;
}

/**
 * @brief 获取指定通道的当前频率配置
 * @param channel_index: 目标通道索引 (0 或 1)
 * @return float: 当前配置的频率值(Hz)。若通道索引无效，则返回0.0f。
 */
float DA_GetFREQ(uint8_t channel_index)
{
    // 参数校验：若通道索引无效，返回一个安全的默认值
    if (channel_index >= NUM_DA_CHANNELS)
        return 0.0f;
    return da_channels[channel_index].frequency;
}

/**
 * @brief 设置指定通道的幅度 (仅更新内存配置)
 * @param channel_index: 目标通道索引 (0 或 1)
 * @param amp: 幅度值 (无单位，与DAC分辨率相关)
 */
void DA_SetAmp(uint8_t channel_index, uint16_t amp)
{
    if (channel_index >= NUM_DA_CHANNELS)
        return;
    da_channels[channel_index].amplitude = DA_ClampAmplitude(amp);
}

/**
 * @brief 获取指定通道的当前幅度配置
 * @param channel_index: 目标通道索引 (0 或 1)
 * @return uint16_t: 当前配置的幅度值。若通道索引无效，则返回0。
 */
uint16_t DA_GetAmp(uint8_t channel_index)
{
    if (channel_index >= NUM_DA_CHANNELS)
        return 0;
    return da_channels[channel_index].amplitude;
}

/**
 * @brief 设置指定通道的相位 (仅更新内存配置)
 * @param channel_index: 目标通道索引 (0 或 1)
 * @param angle: 相位值 (单位: 度, 0-359)
 */
void DA_SetPhase(uint8_t channel_index, uint16_t angle)
{
    if (channel_index >= NUM_DA_CHANNELS)
        return;
    da_channels[channel_index].phase = angle;
}

/**
 * @brief 获取指定通道的当前相位配置
 * @param channel_index: 目标通道索引 (0 或 1)
 * @return uint16_t: 当前配置的相位值(度)。若通道索引无效，则返回0。
 */
uint16_t DA_GetPhase(uint8_t channel_index)
{
    if (channel_index >= NUM_DA_CHANNELS)
        return 0;
    return da_channels[channel_index].phase;
}

/**
 * @brief 设置指定通道的波形类型 (仅更新内存配置)
 * @param channel_index: 目标通道索引 (0 或 1)
 * @param wave: 波形类型 (枚举 Waveform_t)
 */
void DA_SetWaveform(uint8_t channel_index, Waveform_t wave)
{
    if (channel_index >= NUM_DA_CHANNELS)
        return;
    da_channels[channel_index].waveform = wave;
}

/**
 * @brief 获取指定通道的当前波形类型配置
 * @param channel_index: 目标通道索引 (0 或 1)
 * @return Waveform_t: 当前配置的波形类型。若通道索引无效，则返回WAVE_SINE。
 */
Waveform_t DA_GetWaveform(uint8_t channel_index)
{
    if (channel_index >= NUM_DA_CHANNELS)
        return WAVE_SINE;
    return da_channels[channel_index].waveform;
}

/**
 * @brief 设置指定通道的全部配置参数（推荐的便捷API）
 * @details
 * 此函数通过调用一系列独立的Set函数，一次性更新通道的所有参数。
 * 这种实现方式提升了代码的复用性和可维护性。
 * @param  channel_index: DA通道索引 (0 for DA1, 1 for DA2)
 * @param  freq: 频率 (Hz)
 * @param  amp: 幅度
 * @param  angle: 相位 (0-359度)
 * @param  wave: 波形类型
 */
void DA_SetConfig(uint8_t channel_index, float freq, uint16_t amp, uint16_t angle, Waveform_t wave)
{
    // 委托给各个独立的Set函数来完成配置更新
    DA_SetFREQ(channel_index, freq);
    DA_SetAmp(channel_index, amp);
    DA_SetPhase(channel_index, angle);
    DA_SetWaveform(channel_index, wave);
}

/**
 * @brief 将内存中的配置应用到硬件
 * @details
 * 此函数是连接软件配置与硬件行为的核心。它负责：
 * 1. 请求FPGA暂停波形生成，以确保寄存器更新的原子性和安全性。
 * 2. 从 `da_channels` 数组中读取两个通道的完整配置。
 * 3. 执行必要的数学计算，如频率控制字和相位映射。
 * 4. 将计算结果写入FPGA内对应的硬件寄存器。
 * 5. 将波形类型参数打包写入组合寄存器。
 * 6. 请求FPGA依据新配置重新生成波形。
 * @note  所有通过Set/SetConfig的修改，必须调用此函数后才能在硬件上生效。
 */
void DA_Apply_Settings(void)
{
    // 1. 请求FPGA暂停输出，以安全更新寄存器
    DA_FPGA_STOP();

    // 2. --- 通道0 (DA1) 参数计算与写入 ---
    // 根据NCO(数控振荡器)原理计算32位频率控制字M
    uint32_t M_DA1 = DA_FrequencyToWord(da_channels[0].frequency);
    // 将32位的M值拆分为高16位和低16位写入硬件寄存器
    DA1_H = M_DA1 >> 16;
    DA1_L = M_DA1 & 0x0000FFFF;
    // 写入幅度寄存器
    DA1_AMP = da_channels[0].amplitude;
    // 将0-359度的相位归一化，并映射到128点ROM地址
    DA1_PHASE = DA_PhaseToPoints(da_channels[0].phase);

    // 3. --- 通道1 (DA2) 参数计算与写入 ---
    uint32_t M_DA2 = DA_FrequencyToWord(da_channels[1].frequency);
    DA2_H = M_DA2 >> 16;
    DA2_L = M_DA2 & 0x0000FFFF;
    DA2_AMP = da_channels[1].amplitude;
    DA2_PHASE = DA_PhaseToPoints(da_channels[1].phase);

    // 4. --- 组合寄存器写入 ---
    // 将两个通道的波形类型打包到一个16位寄存器中：高8位为通道1，低8位为通道0
    // 直接赋值可避免读-改-写操作的潜在风险
    DA_WAVEFORM = (da_channels[1].waveform << 8) | (da_channels[0].waveform);

    // 5. 请求FPGA依据新配置重新生成波形
    DA_FPGA_START();
}
