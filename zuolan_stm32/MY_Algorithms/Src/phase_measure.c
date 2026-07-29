/*******************************************************************************
 * @file      phase_measure.c
 * @author    左岚
 * @brief     旧版单周期过零相位算法
 * @note      `lengh`必须等于一个完整周期的采样点数。该接口未用于当前
 * Key4双路相位流程；Key4在`ad_measure.c`中使用基波复数投影和采样时差补偿。
 *******************************************************************************/

#include "phase_measure.h"

// 旧版接口保留的全局结果，单位为度。
float phase_diff;

/**
 * @brief  使用过零点检测法计算两个信号的相位差
 * @param  fifo_data1_f 指向信号1的采样数据数组
 * @param  fifo_data2_f 指向信号2的采样数据数组
 * @param  lengh        (应为length) 采样数据的长度，代表一个信号周期
 * @retval None (结果直接存入全局变量 `phase_diff`)
 */
void calculate_phase_diff(float *fifo_data1_f, float *fifo_data2_f, int lengh)
{
    int zero_cross1 = -1, zero_cross2 = -1;
    int i;

    // 查找两路第一个上升过零点。
    for (i = 1; i < lengh; i++)
    {
        if (zero_cross1 == -1 && fifo_data1_f[i] > 0 && fifo_data1_f[i - 1] <= 0)
        {
            zero_cross1 = i;
        }

        if (zero_cross2 == -1 && fifo_data2_f[i] > 0 && fifo_data2_f[i - 1] <= 0)
        {
            zero_cross2 = i;
        }

        if (zero_cross1 != -1 && zero_cross2 != -1)
        {
            break;
        }
    }

    if (zero_cross1 == -1 || zero_cross2 == -1)
    {
        phase_diff = 0.0f;
        return;
    }

    int delta_t = zero_cross2 - zero_cross1;

    // lengh代表一个周期的采样点数。
    phase_diff = ((float)delta_t / (float)lengh) * 360.0f;

    // 归一化到[-180°, 180°]。
    if (phase_diff > 180.0f)
    {
        phase_diff -= 360.0f;
    }
    else if (phase_diff < -180.0f)
    {
        phase_diff += 360.0f;
    }
}
