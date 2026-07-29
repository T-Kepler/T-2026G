/*******************************************************************************
 * @file      my_fft.h
 * @author    左岚
 * @version   V1.1
 * @date      2025-07-18
 * @brief     FFT频谱分析模块的头文件
 * @note      此文件定义了FFT模块的配置（如点数），声明了所需的全局数据
 * 缓冲区，并提供了所有公共函数的外部接口。
 *******************************************************************************/

#ifndef __MY_FFT_H
#define __MY_FFT_H

#include "bsp_system.h"
#include "arm_math.h" // 包含ARM CMSIS-DSP库，用于FFT相关函数

// --------------------------- 配置宏定义 ---------------------------

// 定义FFT的计算点数。
// 该值必须是2的幂次方。对于本工程使用的基4(Radix-4)算法，最好是4的幂次方(如 64, 256, 1024)。
#define FFT_LENGTH 1024
#define FFT_COARSE_LENGTH 128

// --------------------------- 全局变量声明 ---------------------------
// 使用extern关键字声明全局变量，这些变量在my_fft.c中定义，可供其他模块访问。

extern float fft_input_buffer[FFT_LENGTH * 2]; // FFT输入复数缓冲区 (实部、虚部交错存储)
extern float fft_magnitude[FFT_LENGTH];        // 存储FFT计算后的幅度谱

// --------------------------- 外部函数声明 ---------------------------

/** @brief FFT模块初始化函数声明 */
void fft_init(void);

/** @brief 使用Hann窗计算FFT频谱并完成幅度补偿。 */
void calculate_fft_spectrum(const float *input_data, uint16_t data_length);

/** @brief 获取指定基波频点的复数FFT相位；信号幅度过低时返回0。 */
int fft_get_phase(const float *input_data, float sampling_freq,
                  float signal_freq, float *phase_deg);

/** @brief 对128点非相干采样做Hann窗FFT，返回主峰频率和峰值幅度。 */
int fft_find_dominant_frequency(const float *input_data, float sampling_freq,
                                float *frequency_hz, float *magnitude);

/** @brief 通过串口输出频谱分析结果 */
void output_fft_spectrum(float sampling_freq);

/** @brief 使用抛物线插值估计峰值频率 */
float get_precise_peak_frequency(float sampling_freq);

/** @brief 计算总谐波失真(THD)函数声明 */
float calculate_thd(float fundamental_freq, float sampling_freq);

/** @brief 计算THD+N（总谐波失真加噪声）函数声明 */
float calculate_thd_n(float fundamental_freq, float sampling_freq);

/** @brief 计算信纳比(SINAD)函数声明 */
float calculate_sinad(float fundamental_freq, float sampling_freq);

#endif /* __MY_FFT_H */
