/**
 * @file   da_output.h
 * @brief   双通道DA波形发生器驱动的公共接口
 * @details  本头文件定义了与DA输出模块交互所需的所有公共API，包括
 * 数据结构、枚举类型和函数原型。任何需要控制DA输出的
 * 其他模块，都应包含此文件以使用其功能。
 * @author  左岚
 * @date   2025-07-20
 * @version  1.1
 */
#ifndef __DA_OUTPUT_H__
#define __DA_OUTPUT_H__

#include "commond_init.h" // 引入通用的初始化和基本类型定义
#include "bsp_system.h"   // 引入板级支持包的系统定义

// =================================================================================
// 1. 公共类型与宏定义
// =================================================================================

/**
 * @brief DA输出的波形类型枚举
 * @details 使用枚举而非宏定义来表示波形，可以提供更强的类型安全和代码可读性。
 * 这些枚举值通常直接对应FPGA寄存器中的设定值。
 */
typedef enum
{
    WAVE_SINE = 0,     ///< 正弦波
    WAVE_SQUARE = 1,   ///< 方波
    WAVE_TRIANGLE = 2, ///< 三角波
    WAVE_SAWTOOTH = 3, ///< 锯齿波
    WAVE_CUSTOM = 4    ///< STM32经FMC写入的128点自定义波形
} Waveform_t;

/** @brief STM32内置并通过FMC上传的预置波形，不直接写入FPGA选择寄存器。 */
typedef enum
{
    DA_PRESET_SINE = 0,
    DA_PRESET_SQUARE,
    DA_PRESET_TRIANGLE,
    DA_PRESET_DEVIATION,
    DA_PRESET_COUNT
} DA_PresetWaveform_t;

/**
 * @brief 单个DA通道的配置参数结构体
 * @details 此结构体聚合了单个DA通道的所有可配置参数，作为硬件寄存器
 * 在内存中的“影子”副本，便于集中管理和传递。
 */
typedef struct
{
    float frequency;     ///< 输出频率 (单位: Hz)
    uint16_t amplitude;  ///< 幅度 (与DAC分辨率相关的无单位数值)
    uint16_t phase;      ///< 相位 (单位: 度, 范围: 0-359)
    Waveform_t waveform; ///< 波形类型 (枚举 Waveform_t)
} DA_Channel_t;

/**
 * @brief 定义系统中DA通道的总数
 * @details 使用宏定义来指定DA通道数量，便于代码移植和维护。
 */
#define NUM_DA_CHANNELS 2
// =================================================================================
// 2. 函数原型声明
// =================================================================================

/**
 * @brief 初始化DA模块
 * @details 该函数应在系统启动流程中调用一次，为所有DA通道设置已知的初始状态。
 */
void DA_Init(void);

/**
 * @brief 将内存中的配置应用到硬件
 * @details 在通过任何Set函数修改配置后，必须调用此函数，才能将更改写入
 * FPGA硬件寄存器使其生效。
 * @note  这是连接软件配置与硬件行为的关键函数。
 */
void DA_Apply_Settings(void);

/**
 * @brief 将STM32内的预置波形数组写入FPGA并切换双路输出
 * @param wave: DA_PRESET_SINE～DA_PRESET_DEVIATION
 */
void DA_LoadPresetWaveform(DA_PresetWaveform_t wave);

// ------------------- 完整配置设置 (推荐方式) -------------------
/**
 * @brief  设置指定DA通道的全部配置参数
 * @details 此便捷API通过调用一系列独立的Set函数来更新内存中的配置。
 * @note  此函数仅更新内存，需调用 DA_Apply_Settings() 应用到硬件。
 * @param  channel_index: DA通道索引 (0 for DA1, 1 for DA2)
 * @param  freq: 目标频率 (Hz)
 * @param  amp: 目标幅度
 * @param  angle: 目标相位 (0-359度)
 * @param  wave: 目标波形类型
 */
void DA_SetConfig(uint8_t channel_index, float freq, uint16_t amp, uint16_t angle, Waveform_t wave);

// ------------------- 单个参数的Get/Set函数 -------------------

/**
 * @brief 设置指定通道的频率 (仅修改内存)
 * @param channel_index: 目标通道索引 (0 或 1)
 * @param freq: 频率值 (Hz)
 */
void DA_SetFREQ(uint8_t channel_index, float freq);
/**
 * @brief 获取指定通道的当前频率配置
 * @param channel_index: 目标通道索引 (0 或 1)
 * @return float: 当前配置的频率值(Hz)。若通道无效则返回0.0f。
 */
float DA_GetFREQ(uint8_t channel_index);

/**
 * @brief 设置指定通道的幅度 (仅修改内存)
 * @param channel_index: 目标通道索引 (0 或 1)
 * @param amp: 幅度值
 */
void DA_SetAmp(uint8_t channel_index, uint16_t amp);
/**
 * @brief 获取指定通道的当前幅度配置
 * @param channel_index: 目标通道索引 (0 或 1)
 * @return uint16_t: 当前配置的幅度值。若通道无效则返回0。
 */
uint16_t DA_GetAmp(uint8_t channel_index);

/**
 * @brief 设置指定通道的相位 (仅修改内存)
 * @param channel_index: 目标通道索引 (0 或 1)
 * @param angle: 相位值 (0-359度)
 */
void DA_SetPhase(uint8_t channel_index, uint16_t angle);
/**
 * @brief 获取指定通道的当前相位配置
 * @param channel_index: 目标通道索引 (0 或 1)
 * @return uint16_t: 当前配置的相位值(度)。若通道无效则返回0。
 */
uint16_t DA_GetPhase(uint8_t channel_index);

/**
 * @brief 设置指定通道的波形 (仅修改内存)
 * @param channel_index: 目标通道索引 (0 或 1)
 * @param wave: 波形类型枚举
 */
void DA_SetWaveform(uint8_t channel_index, Waveform_t wave);
/**
 * @brief 获取指定通道的当前波形配置
 * @param channel_index: 目标通道索引 (0 或 1)
 * @return Waveform_t: 当前配置的波形类型。若通道无效则返回WAVE_SINE。
 */
Waveform_t DA_GetWaveform(uint8_t channel_index);

#endif // __DA_OUTPUT_H__
