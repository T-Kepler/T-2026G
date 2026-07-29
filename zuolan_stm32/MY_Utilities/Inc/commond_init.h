/**
 * @file commond_init.h
 * @brief 项目通用初始化与宏定义头文件
 * @details
 * 该文件是整个项目的核心基础头文件之一，包含了被广泛使用的定义：
 * 1.  常用数据类型的别名，用以简化代码书写、增强可移植性。
 * 2.  基于Cortex-M4内核位带(Bit-Band)操作的GPIO快速访问宏，实现高效I/O。
 * 3.  通过FSMC/FMC与FPGA通信的硬件寄存器地址映射。
 * 4.  FPGA主控制寄存器中各个控制位的功能宏定义。
 * 5.  系统中使用的重要常量，如FIFO大小、时钟频率等。
 *
 * @author 左岚
 * @date 2025-07-18
 */
#ifndef __COMMOND_INIT_H__
#define __COMMOND_INIT_H__

// 引入STM32F4系列的主头文件，提供了所有外设寄存器、数据类型等基础定义。
#include "stm32f4xx.h"

//================================================================================
// 1. 数据类型别名定义
//================================================================================
/**
 * @brief 定义一系列常用的数据类型短关键字，提高代码可读性和编写效率。
 * @note 命名约定:
 * s/u: signed/unsigned (有/无符号)
 * 8/16/32: 位宽
 * c: const (常量)
 * v: volatile (易失的, STM32标准库中 __IO 表示可读写, __I 表示只读)
 */
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef const int32_t sc32;
typedef const int16_t sc16;
typedef const int8_t sc8;

typedef __IO int32_t vs32;
typedef __IO int16_t vs16;
typedef __IO int8_t vs8;

typedef __I int32_t vsc32;
typedef __I int16_t vsc16;
typedef __I int8_t vsc8;

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef const uint32_t uc32;
typedef const uint16_t uc16;
typedef const uint8_t uc8;

typedef __IO uint32_t vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t vu8;

typedef __I uint32_t vuc32;
typedef __I uint16_t vuc16;
typedef __I uint8_t vuc8;

//================================================================================
// 2. 位带(Bit-Band)操作宏
//================================================================================
/**
 * @brief Cortex-M4内核的位带操作，实现对单个GPIO引脚的原子操作，以避免“读-改-写”指令带来的潜在问题。
 */
// 计算指定外设地址的某个位在位带别名区的地址
#define BITBAND(addr, bitnum) ((addr & 0xF0000000) + 0x2000000 + ((addr & 0xFFFFF) << 5) + (bitnum << 2))
// 将一个地址强制转换为一个可操作的指针
#define MEM_ADDR(addr) *((volatile unsigned long *)(addr))
// 获取指定外设地址指定位的别名区地址指针，直接对此指针读写即等效于对原始位的操作
#define BIT_ADDR(addr, bitnum) MEM_ADDR(BITBAND(addr, bitnum))

//================================================================================
// 3. GPIO快速访问宏 (基于位带操作)
//================================================================================
#define GPIO_ODR_OFFSET 20 // GPIO 输出数据寄存器(ODR)相对于端口基地址的偏移量 (0x14)
#define GPIO_IDR_OFFSET 16 // GPIO 输入数据寄存器(IDR)相对于端口基地址的偏移量 (0x10)

// 计算指定GPIO端口的ODR和IDR寄存器的绝对地址
#define GPIO_ODR_ADDR(port) ((port##_BASE) + GPIO_ODR_OFFSET)
#define GPIO_IDR_ADDR(port) ((port##_BASE) + GPIO_IDR_OFFSET)

// 通用宏：获取指定GPIO端口某个引脚的位带操作地址
#define GPIOx_OUT(port, n) BIT_ADDR(GPIO_ODR_ADDR(port), n) // 获取输出引脚的位带地址
#define GPIOx_IN(port, n) BIT_ADDR(GPIO_IDR_ADDR(port), n)  // 获取输入引脚的位带地址

/**
 * @brief 为每个GPIO端口定义独立的快速访问宏。
 * @example
 * `PAout(5) = 1;` // 将PA5引脚设置为高电平
 * `if (PBin(3) == 0)` // 判断PB3引脚是否为低电平
 */
#define PAout(n) GPIOx_OUT(GPIOA, n)
#define PAin(n) GPIOx_IN(GPIOA, n)

#define PBout(n) GPIOx_OUT(GPIOB, n)
#define PBin(n) GPIOx_IN(GPIOB, n)

#define PCout(n) GPIOx_OUT(GPIOC, n)
#define PCin(n) GPIOx_IN(GPIOC, n)

#define PDout(n) GPIOx_OUT(GPIOD, n)
#define PDin(n) GPIOx_IN(GPIOD, n)

#define PEout(n) GPIOx_OUT(GPIOE, n)
#define PEin(n) GPIOx_IN(GPIOE, n)

// 当前项目只为实际使用的 GPIOA～GPIOE 提供快速访问宏。

//================================================================================
// 4. FPGA主控制寄存器(CTRL_DATA)位定义
//================================================================================
/**
 * @brief 定义了FPGA全局控制寄存器CTRL_DATA中每个位的具体功能。
 * @note 使用位移枚举，代码更清晰，便于理解每个标志位的位置。
 */
enum FPGA_CTRL_BITS
{
    DA_FREQ_EN = (1 << 0), // Bit 0:  DA波形输出总使能 (1:启动)
    AD_SYNC_MODE = (1 << 1),   // Bit 1:  同步装载双NCO初相并启用固定样本配对
    AD1_FREQ_EN = (1 << 2),    // Bit 2:  AD1采样时钟使能 (1:启动)
    AD2_FREQ_EN = (1 << 3),    // Bit 3:  AD2采样时钟使能 (1:启动)
    AD1_FIFO_WR = (1 << 4),    // Bit 4:  AD1数据写入FIFO使能 (1:允许写入)
    AD1_FIFO_RD = (1 << 5),    // Bit 5:  MCU从AD1 FIFO读取数据使能 (1:允许读取)
    AD2_FIFO_WR = (1 << 6),    // Bit 6:  AD2数据写入FIFO使能 (1:允许写入)
    AD2_FIFO_RD = (1 << 7),    // Bit 7:  MCU从AD2 FIFO读取数据使能 (1:允许读取)
    AD1_FREQ_CLR = (1 << 8),   // Bit 8:  AD1测频计数器清零控制 (0:清除, 1:正常工作, 低电平有效)
    AD1_FREQ_START = (1 << 9), // Bit 9:  AD1测频过程启动控制 (1:启动计数)
    AD2_FREQ_CLR = (1 << 10),  // Bit 10: AD2测频计数器清零控制 (0:清除, 1:正常工作, 低电平有效)
    AD2_FREQ_START = (1 << 11),// Bit 11: AD2测频过程启动控制 (1:启动计数)
    AD1_FIFO_SHORT = (1 << 12),// Bit 12: AD1短采集模式（128点）
    AD2_FIFO_SHORT = (1 << 13),// Bit 13: AD2短采集模式（128点）
    AD1_FIFO_RESET = (1 << 14),// Bit 14: AD1 FIFO异步复位（高电平有效）
    AD2_FIFO_RESET = (1 << 15) // Bit 15: AD2 FIFO异步复位（高电平有效）
};

//================================================================================
// 5. FPGA 寄存器地址映射
//================================================================================
/**
 * @brief 通过FSMC/FMC将FPGA内部寄存器映射到STM32的内存地址空间。
 * @details
 * - 基地址: 0x64000000
 * - 数据宽度: 16位
 * - 核心设计: 许多地址在“写”和“读”操作时对应FPGA内部不同的寄存器，
 * 这是一种节省地址空间的常用设计。
 */
// 基础宏：计算FPGA寄存器的绝对地址。基地址0x64000000, `addr`是16位寄存器索引。
#define reg_addr(addr) ((uint32_t *)(0x64000000 + ((addr) << 1)))

/** @name 寄存器地址定义 */
///@{

/** @brief 地址索引0：G题采集控制，bit0=AD1硬件长帧采集。 */
#define G_CAPTURE_CTRL *(vu16 *)reg_addr(0)
#define G_CAPTURE_LONG_AD1 (1U << 0)

/** @brief 地址索引 1: 全局控制寄存器（写入并支持回读） */
#define CTRL_DATA *(vu16 *)reg_addr(1)

/** @brief 地址索引 2, 3: */
// 写操作: DA1 输出频率控制字 (32位)
#define DA1_H *(vu16 *)reg_addr(2)
#define DA1_L *(vu16 *)reg_addr(3)
// 读操作: AD1 频率测量基准时钟(base)的计数值 (32位)
#define BASE1_FREQ_H *(vu16 *)reg_addr(2)
#define BASE1_FREQ_L *(vu16 *)reg_addr(3)

/** @brief 地址索引 4, 5: */
// 写操作: DA2 输出频率控制字 (32位)
#define DA2_H *(vu16 *)reg_addr(4)
#define DA2_L *(vu16 *)reg_addr(5)
// 读操作: AD2 频率测量基准时钟(base)的计数值 (32位)
#define BASE2_FREQ_H *(vu16 *)reg_addr(4)
#define BASE2_FREQ_L *(vu16 *)reg_addr(5)

/** @brief 地址索引 6, 7: */
// 写操作: AD1 采样频率控制字 (32位)
#define AD1_FS_H *(vu16 *)reg_addr(6)
#define AD1_FS_L *(vu16 *)reg_addr(7)
// 读操作: AD1 FIFO数据和采集完成标志
#define AD1_DATA_SHOW *(vu16 *)reg_addr(6) // 读此地址获取AD1 FIFO中的一个数据
#define AD1_FULL_FLAG *(vu16 *)reg_addr(7) // 读此地址获取AD1采集完成标志

/** @brief 地址索引 8, 9: */
// 写操作: AD2 采样频率控制字 (32位)
#define AD2_FS_H *(vu16 *)reg_addr(8)
#define AD2_FS_L *(vu16 *)reg_addr(9)
// 读操作: AD2 FIFO数据和采集完成标志
#define AD2_DATA_SHOW *(vu16 *)reg_addr(8) // 读此地址获取AD2 FIFO中的一个数据
#define AD2_FULL_FLAG *(vu16 *)reg_addr(9) // 读此地址获取AD2采集完成标志

/** @brief 地址索引 10, 11: */
// 写操作: DA1, DA2 相位控制字
#define DA1_PHASE *(vu16 *)reg_addr(10)
#define DA2_PHASE *(vu16 *)reg_addr(11)
// 读操作: AD1 频率测量被测信号的计数值 (32位)
#define AD1_FREQ_H *(vu16 *)reg_addr(10)
#define AD1_FREQ_L *(vu16 *)reg_addr(11)

/** @brief 地址索引 12, 13: */
// 写操作: DA1, DA2 波形选择
#define DA_WAVEFORM *(vu16 *)reg_addr(12) // 低8位=DA1, 高8位=DA2
#define DA_STEP *(vu16 *)reg_addr(13)     // 低8位=DA1, 高8位=DA2
// 读操作: AD2 频率测量被测信号的计数值 (32位)
#define AD2_FREQ_H *(vu16 *)reg_addr(12)
#define AD2_FREQ_L *(vu16 *)reg_addr(13)

/** @brief 地址索引 14, 15: DA幅度缩放值（只写） */
#define DA1_AMP *(vu16 *)reg_addr(14)
#define DA2_AMP *(vu16 *)reg_addr(15)

/** @brief 0x0100~0x017F: 双路DA共用的128点、14位自定义波形写窗口 */
#define DA_WAVE_RAM(index) *(vu16 *)reg_addr(0x0100U + (index))

///@}

//================================================================================
// 6. 系统级常量定义
//================================================================================

/** @name AD/DA/FFT 模块参数 */
///@{
#define AD_FIFO_SIZE 1024      // AD FIFO 缓冲区的深度（点数）
#define AD_FIFO_SIZE_N 1024.0f // AD FIFO 深度的浮点数形式，用于浮点计算
#define AD_SHORT_SIZE 128       // 自适应测频的短采集点数
#define G_CAPTURE_SIZE 16384U   // G题AD1原始采集点数
#define G_SAMPLE_RATE_HZ 5000000.0f
#define G_FFT_SIZE 4096U
#define G_DECIMATION 4U
#define DA_ROM_SIZE 128        // DA ROM/RAM 缓冲区的深度（点数）
#define FFT_LENGTH 1024        // FFT运算的点数
///@}

/** @name 频率计算相关常数 */
///@{
#define AD_FREQ_CONSTANT 4294967296.0f // AD采样频率设置常数, 等于 2^32
#define DA_FREQ_CONSTANT 4294967296.0  // DA输出频率设置常数, 等于 2^32
#define FPGA_BASE_CLK 150000000.0f     // ADC采样NCO基准时钟 (150 MHz)
#define DA_SAMPLE_CLK 125000000.0f     // AD9767固定采样/DDS时钟 (125 MHz)
#define DA_PHASE_POINTS 128.0f         // FPGA内置DA波形ROM相位点数
#define DA_OUTPUT_MAX_FREQ (DA_SAMPLE_CLK / 2.0f) // DDS奈奎斯特上限
///@}

#endif // __COMMOND_INIT_H__
