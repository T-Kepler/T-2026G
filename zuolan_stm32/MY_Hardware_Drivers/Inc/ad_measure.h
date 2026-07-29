/*******************************************************************************
 * @file      ad_measure.h
 * @author    左岚
 * @version   V1.0
 * @date      2025-07-18
 * @brief     ADC（模数转换器）模块的头文件。
 * @note      此文件定义了ADC数据采集和处理所需的数据结构、外部变量和函数原型。
 * 它支持双通道并行采集，并提供了灵活的频率设置模式，可以根据
 * 信号频率自动计算采样率，或直接指定采样率。
 *******************************************************************************/

#ifndef __AD_MEASUFRE_H__
#define __AD_MEASUFRE_H__

#include "commond_init.h"
#include "cmd_to_fun.h"
#include "bsp_system.h"

// --- 枚举定义 ---

/**
 * @brief  定义传递给ADC配置函数的频率参数的类型。
 * @details 使用此枚举可以明确函数调用时传入的频率值是信号本身的频率，
 * 还是用户期望的ADC采样频率，从而避免了使用“魔术数字”，
 * 使代码意图更清晰，可读性更强。
 */
typedef enum
{
    FREQ_MODE_SIGNAL,   ///< 模式0: 输入值为信号频率。程序将基于此频率计算一个合适的采样率，以保证采样的周期完整性，便于后续FFT等处理。
    FREQ_MODE_SAMPLING  ///< 模式1: 输入值为直接指定的ADC采样频率。程序将直接使用此值配置ADC硬件。
} AdcFrequencyMode;


// --- 外部变量声明 ---
// 这些变量在 ad.c 中定义，此处声明以便在工程的其他文件中访问ADC的最终处理结果。

/**
 * @brief ADC采样数据缓冲区 (浮点数形式)。
 * @details 这两个数组用于存储经过转换的电压值。原始的ADC采样值（通常是12位或16位整数）
 * 由 `vpp_adc_parallel` 完成采集和电压转换后存放在这里。
 * AD_FIFO_SIZE 是在其他地方定义的宏，代表采样点的数量。
 */
extern float fifo_data1_f[AD_FIFO_SIZE], fifo_data2_f[AD_FIFO_SIZE];

/**
 * @brief 存储计算出的电压幅值。
 * @details `vpp_adc_parallel` 对采集数据查找最大最小值后，
 * 计算出的两个通道信号的电压幅值（Amplitude）或峰峰值（Vpp）将保存在这两个变量中。
 */
extern float vol_amp1, vol_amp2;


// --- 函数原型声明 ---

/**
 * @brief  配置并启动一次双通道并行的ADC采集。
 * @param  freq1  通道1的频率设置值。
 * @param  mode1  通道1的频率模式 (信号频率或采样频率)。
 * @param  freq2  通道2的频率设置值。
 * @param  mode2  通道2的频率模式。
 * @retval None
 */
void vpp_adc_parallel(float freq1, AdcFrequencyMode mode1, float freq2, AdcFrequencyMode mode2);

/**
 * @brief 使用ADC直接采样，自适应测量指定通道的输入信号频率。
 * @param channel 通道号（1或2）。
 * @param frequency_hz 成功时写入测得频率。
 * @return 1表示测量成功，0表示无有效信号或参数无效。
 */
int ad_detect_frequency(int channel, float *frequency_hz);

/** @brief 使用当前1024点采集缓冲区的过零点精化信号频率。 */
float ad_estimate_captured_frequency(int channel);

/** @brief 获取指定通道实际写入FPGA的ADC采样频率。 */
float get_adc_sampling_frequency(int channel);

/**
 * @brief  获取指定通道的FFT频率轴采样率。
 * @details 两种模式都返回实际写入FPGA的物理采样率。
 * @param  channel 通道号（1或2）。
 * @return FFT频率轴采样率；通道无效或尚未设置时返回0。
 */
float get_adc_fft_sampling_frequency(int channel);

/**
 * @brief 同步采集双AD并测量相位差。
 * @param signal_freq 两路共同基波频率。
 * @param calibration_ns AD2-AD1固定通道相位误差对应的有符号时间校准量。
 * @param phase_difference_deg 返回AD2-AD1校准后相位，范围(-180, 180]。
 * @return 1表示相位有效，0表示采集失败或任一路信号幅度过低。
 */
int ad_measure_phase_difference(float signal_freq, float calibration_ns,
                                float *phase_difference_deg);

/**
 * @brief 以指定采样率从AD1采集G_CAPTURE_SIZE点，并换算为模块输入电压。
 * @param sampling_freq_hz 期望采样率。
 * @param voltage_data 输出电压数组，容量至少G_CAPTURE_SIZE。
 * @param raw_data 输出12位ADC原始码数组，容量至少G_CAPTURE_SIZE，可为NULL。
 * @param raw_min 返回本帧最小原始码，可为NULL。
 * @param raw_max 返回本帧最大原始码，可为NULL。
 * @return 1表示成功，0表示超时或FMC数据无效。
 */
int ad_capture_g_frame(float sampling_freq_hz, float *voltage_data,
                       u16 *raw_data, u16 *raw_min, u16 *raw_max);

#endif //__AD_MEASUFRE_H__
