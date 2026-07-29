/***************************************************************************************************
 * @file    ad9959.h
 * @brief   AD9959 四通道DDS信号发生器 驱动头文件
 * @author  左岚, 调库侠
 * @version V1.1
 * @date    2025-07-24
 *
 * @attention
 *
 * 本驱动库基于STM32 HAL库编写，通过GPIO模拟SPI总线与AD9959通信。
 * 驱动功能包括：
 * 1. 单点频率、相位、幅度信号输出。
 * 2. 频率、相位、幅度的线性扫描功能（扫频、扫相、扫幅）。
 * 3. ASK, FSK, PSK 数字调制功能。
 * 4. 提供了基于状态机的非阻塞式ASK/FSK数据连续发送控制。
 *
 * SYSCLK (系统时钟) = Crystal * PLL倍数 = 25MHz * 20 = 500MHz
 *
 ***************************************************************************************************/

#ifndef __AD9959_H__
#define __AD9959_H__
#include "main.h"
#include "commond_init.h"
#include "bsp_system.h"

// --------------------------------- 核心参数定义 ---------------------------------

/**
 * @brief 晶振频率定义
 * @note  外部提供给AD9959的晶振为25MHz
 */
#define CRYSTAL_FREQ 25000000UL

/**
 * @brief 系统时钟 (SYSCLK) 频率定义
 * @note  通过内部PLL将25MHz晶振20倍频得到，SYSCLK = 500MHz.
 * FR1[22:18] 设置为 0b10100 (20)
 */
#define SYSCLK_FREQ 500000000UL

/**
 * @brief 频率控制字 (FTW) 计算因子
 * @note  公式: FTW = F_out * (2^32 / SYSCLK)
 * 该宏定义预先计算了 (2^32 / SYSCLK) 的值，以简化运行时计算。
 * ACC_FRE_FACTOR = 4294967296 / 500000000 = 8.589934592
 */
#define ACC_FRE_FACTOR 8.589934592

/**
 * @brief 相位偏移字 (POW) 计算因子
 * @note  公式: POW = Phase_deg * (2^14 / 360)
 * AD9959的相位分辨率为14位 (16384阶)。
 * POW_REF = 16384 / 360.0 = 45.51111111
 */
#define POW_REF (16384.0f / 360.0f)

// --------------------------------- AD9959寄存器地址宏定义 ---------------------------------

#define CSR 0x00   // 通道选择寄存器 (Channel Select Register), 1 byte
#define FR1 0x01   // 功能寄存器1 (Function Register 1), 3 bytes
#define FR2 0x02   // 功能寄存器2 (Function Register 2), 2 bytes
#define CFR 0x03   // 通道功能寄存器 (Channel Function Register), 3 bytes
#define CFTW0 0x04 // 通道频率调谐字0 (Channel Frequency Tuning Word 0), 4 bytes
#define CPOW0 0x05 // 通道相位偏移字0 (Channel Phase Offset Word 0), 2 bytes
#define ACR 0x06   // 幅度控制寄存器 (Amplitude Control Register), 3 bytes
#define LSRR 0x07  // 线性扫描斜率寄存器 (Linear Sweep Ramp Rate), 2 bytes. [15:8]下降, [7:0]上升
#define RDW 0x08   // 上升增量字 (Rising Delta Word), 4 bytes
#define FDW 0x09   // 下降增量字 (Falling Delta Word), 4 bytes
#define CW1 0x0a   // 通道字1 (Channel Word 1), 4 bytes. 可用作Profile 0
#define CW2 0x0b   // 通道字2 (Channel Word 2), 4 bytes. 可用作Profile 1
#define CW3 0x0c   // ...
#define CW4 0x0d
#define CW5 0x0e
#define CW6 0x0f
#define CW7 0x10
#define CW8 0x11
#define CW9 0x12
#define CW10 0x13
#define CW11 0x14
#define CW12 0x15
#define CW13 0x16
#define CW14 0x17
#define CW15 0x18 // 通道字15 (Channel Word 15), 4 bytes. 可用作Profile 14

// Profile寄存器基址 (用于调制功能)
#define PROFILE_ADDR_BASE 0x0A

// --------------------------------- 功能配置宏定义 ---------------------------------

// 通道选择位定义 (写入CSR寄存器)
#define CH0_SELECT 0x10    // 使能通道0
#define CH1_SELECT 0x20    // 使能通道1
#define CH2_SELECT 0x40    // 使能通道2
#define CH3_SELECT 0x80    // 使能通道3
#define ALL_CH_SELECT 0xF0 // 使能所有通道

// FR1[9:8] 调制电平选择位
#define LEVEL_MOD_2 0x00  // 2电平调制 (ASK, FSK, PSK)
#define LEVEL_MOD_4 0x01  // 4电平调制
#define LEVEL_MOD_8 0x02  // 8电平调制
#define LEVEL_MOD_16 0x03 // 16电平调制

// CFR[23:22] 调制模式选择位 (AFP)
#define MOD_DISABLE 0x00 // 禁用调制
#define MOD_ASK 0x40     // 幅度键控 (ASK)
#define MOD_FSK 0x80     // 频率键控 (FSK)
#define MOD_PSK 0xC0     // 相位键控 (PSK)

// CFR[14] 线性扫描使能位
#define SWEEP_ENABLE 0x4000  // 线性扫描使能
#define SWEEP_DISABLE 0x0000 // 线性扫描禁用

// --------------------------------- GPIO控制函数声明 (底层) ---------------------------------
void AD9959_P0_H(void); ///< 拉高P0 (Profile 0 Pin)
void AD9959_P0_L(void); ///< 拉低P0
void AD9959_P1_H(void); ///< 拉高P1 (Profile 1 Pin)
void AD9959_P1_L(void); ///< 拉低P1
void AD9959_P2_H(void); ///< 拉高P2 (Profile 2 Pin)
void AD9959_P2_L(void); ///< 拉低P2
void AD9959_P3_H(void); ///< 拉高P3 (Profile 3 Pin)
void AD9959_P3_L(void); ///< 拉低P3

void AD9959_UP_H(void);  ///< 拉高IO_UPDATE引脚, 触发寄存器更新
void AD9959_UP_L(void);  ///< 拉低IO_UPDATE引脚
void AD9959_PDC_H(void); ///< 拉高PDC (Power-Down Control)引脚, 进入省电模式
void AD9959_PDC_L(void); ///< 拉低PDC引脚, 正常工作
void AD9959_RST_H(void); ///< 拉高RESET引脚
void AD9959_RST_L(void); ///< 拉低RESET引脚, 复位设备

void AD9959_CS_H(void);  ///< 拉高CS (Chip Select)引脚, 取消片选
void AD9959_CS_L(void);  ///< 拉低CS引脚, 使能片选
void AD9959_SCK_H(void); ///< 拉高SCLK (Serial Clock)引脚
void AD9959_SCK_L(void); ///< 拉低SCLK引脚

void AD9959_SDIO0_H(void); ///< 拉高SDIO0数据引脚
void AD9959_SDIO0_L(void); ///< 拉低SDIO0数据引脚
void AD9959_SDIO1_H(void); ///< 拉高SDIO1数据引脚
void AD9959_SDIO1_L(void); ///< 拉低SDIO1数据引脚
void AD9959_SDIO2_H(void); ///< 拉高SDIO2数据引脚
void AD9959_SDIO2_L(void); ///< 拉低SDIO2数据引脚
void AD9959_SDIO3_H(void); ///< 拉高SDIO3数据引脚
void AD9959_SDIO3_L(void); ///< 拉低SDIO3数据引脚

// --------------------------------- 核心功能函数声明 (高层) ---------------------------------

// --- 基础控制函数 ---
void AD9959_Init(void);                 ///< 初始化AD9959设备
void AD9959_Reset(void);                ///< 硬复位AD9959设备
void AD9959_IO_Update(void);            ///< 触发I/O更新, 将缓冲区数据写入活动寄存器
void AD9959_Select_Channel(u8 Channel); ///< 选择要操作的通道

// --- 单点信号输出函数 ---
void AD9959_Set_Freq(u32 Freq_Hz);                                            ///< 为当前选中通道设置输出频率 (单位: Hz)
void AD9959_Set_Phase(float Phase_Deg);                                       ///< 为当前选中通道设置输出相位 (单位: 度)
void AD9959_Set_Amp(u16 Amp);                                                 ///< 为当前选中通道设置输出幅度 (范围: 0-1023)
void AD9959_Single_Output(u8 Channel, u32 Freq_Hz, float Phase_Deg, u16 Amp); ///< 单通道波形输出便捷函数

// --- 线性扫描函数 ---
void AD9959_Set_Linear_Sweep(u8 Channel, u32 cfr_val); ///< 设置线性扫描模式(频率/幅度/相位)
void AD9959_Write_LSRR(u8 rise_srr, u8 fall_srr);      ///< 写线性扫描斜率寄存器 (LSRR)
void AD9959_Write_RDW(u32 rising_delta_word);          ///< 写上升增量字 (RDW)
void AD9959_Write_FDW(u32 falling_delta_word);         ///< 写下降增量字 (FDW)

void AD9959_Set_Freq_Sweep(u8 Channel, u32 start_freq, u32 end_freq, u8 rise_srr, u8 fall_srr, u32 rise_step, u32 fall_step);                       ///< 配置线性扫频
void AD9959_Set_Amp_Sweep(u8 Channel, u32 freq, u16 start_amp, u16 end_amp, u8 rise_srr, u8 fall_srr, u16 rise_step, u16 fall_step);                ///< 配置线性扫幅
void AD9959_Set_Phase_Sweep(u8 Channel, u32 freq, u16 amp, u16 start_phase, u16 end_phase, u8 rise_srr, u8 fall_srr, u16 rise_step, u16 fall_step); ///< 配置线性扫相

// --- 调制(Modulation)相关函数 ---
void AD9959_Modulation_Init(u8 Channel, u8 mod_level, u8 mod_type); ///< 初始化调制功能
void AD9959_Write_Profile_Freq(u8 profile, u32 freq_hz);            ///< 向Profile寄存器写入频率数据
void AD9959_Write_Profile_Amp(u8 profile, u16 amp_data);            ///< 向Profile寄存器写入幅度数据
void AD9959_Write_Profile_Phase(u8 profile, u16 phase_data);        ///< 向Profile寄存器写入相位数据

void AD9959_Set_FSK(u8 Channel, u32 *freq_array);            ///< 配置FSK参数
void AD9959_Set_ASK(u8 Channel, u32 freq, u16 *amp_array);   ///< 配置ASK参数
void AD9959_Set_PSK(u8 Channel, u32 freq, u16 *phase_array); ///< 配置PSK参数

void AD9959_Default_Output_Init(void); ///< 配置CH0单音及CH1/CH2/CH3共享码流调制输出

// --- 数字调制连续发送控制 (非阻塞) ---
void AD9959_Start_ASK_Transmission(void);  ///< 启动ASK连续发送测试序列
void AD9959_Stop_ASK_Transmission(void);   ///< 停止ASK发送
void AD9959_Start_FSK_Transmission(void);  ///< 启动FSK连续发送测试序列
void AD9959_Stop_FSK_Transmission(void);   ///< 停止FSK发送
void AD9959_Modulation_State_Update(void); ///< 调制状态更新函数(需在主循环中周期性调用)
u8 AD9959_Is_ASK_Active(void);             ///< 查询ASK发送任务是否激活
u8 AD9959_Is_FSK_Active(void);             ///< 查询FSK发送任务是否激活

// --- 用户测试函数 ---
void AD9959_proc(void); ///< 用户自定义测试逻辑

#endif // __AD9959_H__
