/**
 * @file freq_measure.c
 * @brief FPGA频率测量驱动程序
 * @details
 * 该文件实现了通过FPGA对两个独立通道（AD1, AD2）进行频率测量的功能。
 * 在固定门控时间内同时统计被测比较器脉冲和150MHz基准脉冲，
 * 再按两者计数比计算输入频率。
 * 主要功能包括：
 * 1. 提供一个通用的频率测量函数，可配置通道。
 * 2. 为每个通道提供独立的测量启动函数。
 * 3. 包含一个周期性处理函数，用于依次更新两个通道的频率测量值。
 *
 * @author 左岚
 * @date 2025-07-18
 */
#include "freq_measure.h"

// *********************************************************************************
// 1. 全局变量定义
// *********************************************************************************

/**
 * @brief 存储通道1和通道2最终计算出的频率值（单位：Hz）
 */
float freq1, freq2;

/**
 * @brief 存储从FPGA读取的原始计数值
 * @details
 * freq_ad1, freq_ad2: 存储在0.1秒门控时间内，被测信号的计数值。
 * freq_base1, freq_base2: 存储在0.1秒门控时间内，150MHz基准时钟的计数值。
 */
u32 freq_ad1, freq_base1;
u32 freq_ad2, freq_base2;

/**
 * @brief 测量频率的通用函数
 * @details
 * 此函数是频率测量的核心。它按以下步骤执行：
 * 1. 清除FPGA内部的计数器，为新的测量做准备。
 * 2. 启动FPGA的计数逻辑。
 * 3. 等待一个固定的门控时间（100毫秒），让计数器在此期间累加。
 * 4. 停止计数。
 * 5. 从FPGA寄存器中读取被测信号和基准时钟的32位计数值（通过组合高16位和低16位寄存器）。
 * 6. 使用公式 `实际频率 = (基准时钟频率 / 基准时钟计数值) * 被测信号计数值` 来计算最终频率。
 * 其中基准时钟频率为150MHz。
 *
 * @param channel 通道号 (1: 对应AD1, 2: 对应AD2)
 * @param freq_ad 指向存储被测信号计数值的变量的指针
 * @param freq_base 指向存储基准时钟计数值的变量的指针
 * @param freq_result 指向存储最终频率计算结果的变量的指针
 */
void fre_measure(int channel, u32 *freq_ad, u32 *freq_base, float *freq_result)
{
    // 步骤1: 清除上一次的采集数据，确保本次测量从0开始
    AD_FREQ_CLR_ENABLE(channel);
    HAL_Delay(1); // 短暂延时确保FPGA命令执行
    AD_FREQ_CLR_DISABLE(channel);
    HAL_Delay(1);

    // 步骤2: 启动FPGA进行频率采集
    AD_FREQ_START(channel);
    HAL_Delay(100); // 核心步骤：设置100ms的门控时间（Gate Time）进行计数
    AD_FREQ_STOP(channel);
    HAL_Delay(1); // 等待停止命令跨入比较器输入时钟域，确保32位计数值稳定

    // 步骤3: 读取被测信号的计数值
    // 通过三元运算符根据通道号选择对应的FPGA寄存器
    u32 freq_h = (channel == 1) ? AD1_FREQ_H : AD2_FREQ_H;
    HAL_Delay(1);
    u32 freq_l = (channel == 1) ? AD1_FREQ_L : AD2_FREQ_L;
    HAL_Delay(1);
    // 将读取到的高16位和低16位数据合并成一个32位整数
    *freq_ad = (freq_h << 16) + freq_l;

    // 步骤4: 读取基准时钟的计数值
    u32 base_h = (channel == 1) ? BASE1_FREQ_H : BASE2_FREQ_H;
    HAL_Delay(1);
    u32 base_l = (channel == 1) ? BASE1_FREQ_L : BASE2_FREQ_L;
    HAL_Delay(1);
    // 合并高低16位数据
    *freq_base = (base_h << 16) + base_l;

    // 步骤5: 数据有效性检查
    // 无被测脉冲或100ms门控内基准计数低于14,000,000时判为无效。
    if (*freq_ad == 0 || *freq_base <= 14000000.0)
    {
        return;
    }

    // 步骤6: 计算实际频率
    // 公式: F_actual = F_base * (N_ad / N_base)
    // F_base = 150,000,000 Hz
    // N_ad = *freq_ad (被测信号计数值)
    // N_base = *freq_base (基准时钟计数值)
    *freq_result = FPGA_BASE_CLK / *freq_base * *freq_ad;
}

/**
 * @brief 测量 AD1 通道的频率
 * @details
 * 这是一个封装函数，专门用于测量通道1的频率。
 * 它调用通用的 `fre_measure` 函数，并传入与通道1相关的全局变量地址。
 */
void fre_measure_ad1(void)
{
    fre_measure(1, &freq_ad1, &freq_base1, &freq1);
}

/**
 * @brief 测量 AD2 通道的频率
 * @details
 * 这是一个封装函数，专门用于测量通道2的频率。
 * 它调用通用的 `fre_measure` 函数，并传入与通道2相关的全局变量地址。
 */
void fre_measure_ad2(void)
{
    fre_measure(2, &freq_ad2, &freq_base2, &freq2);
}

/**
 * @brief 频率测量主处理函数
 * @details
 * 此函数按顺序调用两个通道的测量函数，以更新它们的频率值。
 * 通常在主循环或定时器中断中周期性地调用此函数，以实现持续的频率监控。
 */
void freq_proc(void)
{
    fre_measure_ad1(); // 测量AD1的频率
    fre_measure_ad2(); // 测量AD2的频率
	  my_printf(&huart1, "fre: %f\r\n", freq2);
}
