/*******************************************************************************
 * @file      phase_measure.h
 * @author    左岚
 * @brief     旧版单周期过零相位算法接口
 * @note      当前Key4双路相位流程不使用此接口。
 *******************************************************************************/

#ifndef __PHASE_MEASURE_H__
#define __PHASE_MEASURE_H__

/**
 * @brief 全局变量，用于存储计算出的相位差（单位：角度）。
 * @details 该变量由 `calculate_phase_diff` 函数直接写入结果。
 */
extern float phase_diff;

/**
 * @brief  使用单周期上升过零点计算相位差
 * @param  fifo_data1_f 指向信号1的采样数据数组
 * @param  fifo_data2_f 指向信号2的采样数据数组
 * @param  lengh        一个完整周期的采样点数；名称为兼容旧接口而保留
 */
void calculate_phase_diff(float *fifo_data1_f, float *fifo_data2_f, int lengh);

#endif // __PHASE_MEASURE_H__
