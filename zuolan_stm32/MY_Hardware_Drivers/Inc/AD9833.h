/**
 ******************************************************************************
 * @file    AD9833.h
 * @author  左岚、调库侠
 * @brief   AD9833波形发生器驱动程序的头文件。
 * 该文件包含了控制AD9833所需的宏定义、寄存器地址和函数原型。
 * 此驱动支持控制一个AD9833设备。
 ******************************************************************************
 */
#ifndef _AD9833_H
#define _AD9833_H

#include "main.h"

/******************************************************************************
 * 宏定义                                                                      *
 ******************************************************************************/

/* 波形模式选择掩码 */
#define AD9833_OUT_SINUS ((0 << 5) | (0 << 1) | (0 << 3))    // 正弦波输出
#define AD9833_OUT_TRIANGLE ((0 << 5) | (1 << 1) | (0 << 3)) // 三角波输出
#define AD9833_OUT_MSB ((1 << 5) | (0 << 1) | (1 << 3))      // 方波输出 (DAC最高有效位)
#define AD9833_OUT_MSB2 ((1 << 5) | (0 << 1) | (0 << 3))     // 方波输出 (DAC最高有效位 / 2)

/* 寄存器地址掩码 */
#define AD9833_REG_CMD (0 << 14)    // 命令寄存器
#define AD9833_REG_FREQ0 (1 << 14)  // 频率寄存器0
#define AD9833_REG_FREQ1 (2 << 14)  // 频率寄存器1
#define AD9833_REG_PHASE0 (6 << 13) // 相位寄存器0
#define AD9833_REG_PHASE1 (7 << 13) // 相位寄存器1

/* 命令控制位 */
#define AD9833_B28 (1 << 13)        // 启用28位频率寄存器写入 (两次连续的14位写入)
#define AD9833_HLB (1 << 12)        // 控制写入28位频率寄存器的哪个部分 (本驱动中未使用)
#define AD9833_FSEL0 (0 << 11)      // 选择频率寄存器0用于输出
#define AD9833_FSEL1 (1 << 11)      // 选择频率寄存器1用于输出
#define AD9833_PSEL0 (0 << 10)      // 选择相位寄存器0用于输出
#define AD9833_PSEL1 (1 << 10)      // 选择相位寄存器1用于输出
#define AD9833_PIN_SW (1 << 9)      // 引脚切换使能 (本驱动中未使用)
#define AD9833_RESET (1 << 8)       // 复位内部寄存器为0
#define AD9833_CLEAR_RESET (0 << 8) // 清除复位状态
#define AD9833_SLEEP1 (1 << 7)      // 禁用内部MCLK
#define AD9833_SLEEP12 (1 << 6)     // 禁用DAC
#define AD9833_OPBITEN (1 << 5)     // 输出位使能 (用于方波模式)
#define AD9833_SIGN_PIB (1 << 4)    // 符号位输出 (本驱动中未使用)
#define AD9833_DIV2 (1 << 3)        // DAC输出除以2 (用于方波模式)
#define AD9833_MODE (1 << 1)        // 设置此位时选择三角波模式

/******************************************************************************
 * AD9833设备的函数原型                                                        *
 ******************************************************************************/

/**
 * @brief  初始化AD9833设备所需的GPIO引脚。
 * @param  无
 * @retval 无
 */
void AD9833_Init(void);

/**
 * @brief  简单的软件延时。
 * @param  无
 * @retval 无
 */
static void AD9833_Delay(void);

/**
 * @brief  通过模拟SPI向AD9833写入16位数据。
 * @param  txdata: 要发送的16位数据。
 * @retval 无
 */
void AD9833_WriteData(uint16_t txdata);

/**
 * @brief  设置AD9833的频率。
 * @param  reg: 目标频率寄存器 (AD9833_REG_FREQ0 或 AD9833_REG_FREQ1)。
 * @param  fout: 要设置的频率值 (单位: Hz)。
 * @retval 无
 */
void AD9833_SetFrequency(unsigned short reg, double fout);

/**
 * @brief  设置AD9833的相位。
 * @param  reg: 目标相位寄存器 (AD9833_REG_PHASE0 或 AD9833_REG_PHASE1)。
 * @param  val: 要设置的相位值 (0-4095)。
 * @retval 无
 */
void AD9833_SetPhase(unsigned short reg, unsigned short val);

/**
 * @brief  为AD9833选择输出波形、频率源和相位源。
 * @param  WaveMode: 波形类型 (例如 AD9833_OUT_SINUS)。
 * @param  Freq_SFR: 选择的频率寄存器 (AD9833_FSEL0 或 AD9833_FSEL1)。
 * @param  Phase_SFR: 选择的相位寄存器 (AD9833_PSEL0 或 AD9833_PSEL1)。
 * @retval 无
 */
void AD9833_SetWave(unsigned int WaveMode, unsigned int Freq_SFR, unsigned int Phase_SFR);

/**
 * @brief  一站式配置AD9833的输出。
 * @param  Freq_SFR:  目标频率寄存器 (AD9833_REG_FREQ0 或 AD9833_REG_FREQ1)。
 * @param  Freq:      要设置的频率值 (单位: Hz)。
 * @param  Phase_SFR: 目标相位寄存器 (AD9833_REG_PHASE0 或 AD9833_REG_PHASE1)。
 * @param  Phase:     要设置的相位值 (0-4095)。
 * @param  WaveMode:  要输出的波形类型 (例如 AD9833_OUT_SINUS)。
 * @retval 无
 */
void AD9833_Setup(unsigned int Freq_SFR, double Freq, unsigned int Phase_SFR, unsigned int Phase, unsigned int WaveMode);

#endif /* _AD9833_H */
