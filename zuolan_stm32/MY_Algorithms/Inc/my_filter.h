/*******************************************************************************
 * @file      my_filter.h
 * @author    左岚
 * @version   V1.0
 * @date      2025-07-18
 * @brief     FIR数字滤波器模块的头文件
 * @note      此文件声明了FIR低通滤波器的外部接口函数。
 *******************************************************************************/

#ifndef __MY_FILTER_H__
#define __MY_FILTER_H__

/**
 * @brief  FIR低通滤波器函数声明
 * @param  input_array  指向输入数据数组的指针
 * @param  length       输入数据的总长度
 * @param  output_array 指向用于存储滤波后数据的数组指针
 * @see    my_filter.c 文件中的具体实现
 */
void arm_fir_f32_lp(const float *input_array, int length, float *output_array);

#endif // __MY_FILTER_H__
