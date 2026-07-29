/*******************************************************************************
 * @file      kalman.c
 * @author    左岚
 * @version   V1.0
 * @date      2025-07-18
 * @brief     一维卡尔曼滤波器算法实现
 * @note      提供单状态滤波迭代，以及10路和单路两个带重置标志的封装。
 *******************************************************************************/

#include "kalman.h"

/**
 * @brief  初始化一个卡尔曼滤波器状态
 * @param  state 指向要初始化的滤波器状态结构体的指针
 * @param  init_x 状态的初始估计值
 * @param  init_p 初始估计的协方差（表示初始估计值的不确定性）
 * @retval None
 */
void Kalman_init(kalman_state *state, float init_x, float init_p)
{
    state->x = init_x;          // x(0|0): 状态的初始值
    state->p = init_p;          // p(0|0): 初始协方差
    state->A = 1;               // 状态转移矩阵 A。对于一维恒定值模型，A=1，表示我们预测下一时刻的值等于当前值。
    state->H = 1;               // 观测矩阵 H。对于直接测量模型，H=1，表示测量值直接对应状态值。
    state->Q = 0.03445;         // 过程噪声协方差 Q。代表模型本身的不确定性或噪声，值越小表示越信任模型预测。
    state->R = 0.07;            // 测量噪声协方差 R。代表测量值（传感器）的噪声，值越小表示越信任本次测量值。
}

/**
 * @brief  执行一次卡尔曼滤波迭代
 * @param  state 滤波器的状态结构体指针
 * @param  z_measure 本次循环的测量值
 * @retval (float) 本次更新后的状态估计值
 */
float kalman_filter(kalman_state *state, float z_measure)
{
    /* 1. 预测(Prediction)阶段 - 根据上一时刻状态预测当前时刻状态 */
    // 状态预测方程: x(k|k-1) = A * x(k-1|k-1)
    state->x = state->A * state->x;
    // 协方差预测方程: p(k|k-1) = A * p(k-1|k-1) * A' + Q  (A'为A的转置, 在一维情况下 A'=A)
    state->p = state->A * state->A * state->p + state->Q;

    /* 2. 更新(Update)阶段 - 结合当前测量值修正预测结果 */
    // 计算卡尔曼增益 K: K(k) = p(k|k-1) * H' / (H * p(k|k-1) * H' + R)
    state->k = state->p * state->H / (state->p * state->H * state->H + state->R);
    // 更新状态估计（最优估计）: x(k|k) = x(k|k-1) + K(k) * (z(k) - H * x(k|k-1))
    state->x = state->x + state->k * (z_measure - state->H * state->x);
    // 更新协方差: p(k|k) = (I - K(k) * H) * p(k|k-1)  (I为单位矩阵, 在一维情况下 I=1)
    state->p = (1 - state->k * state->H) * state->p;

    return state->x;
}


/*******************************************************************************
 * 卡尔曼滤波器应用封装
 *******************************************************************************/

// 定义全局变量，用于控制 `kalman` 函数的重置逻辑
uint8_t last = 1, current = 0;
// 定义一个包含10个卡尔曼滤波器状态的数组，可用于对10个独立的数据流进行滤波
kalman_state state[10];

/**
 * @brief  针对一组数据流的卡尔曼滤波封装函数
 * @details 此函数管理一个包含10个滤波器的阵列。它通过一个外部标志位 `current` 和 `last`
 * 来控制所有10个滤波器的重置。
 * @param  i  要使用的滤波器在 `state` 数组中的索引 (0-9)。
 * @param  z_measure  当前的测量值。
 * @retval (float) 滤波后的值或初始值。
 */
float kalman(uint8_t i, float z_measure)
{
    // 检查重置标志。当外部代码改变 `current` 的值使其不等于 `last` 时，触发重置。
    if (current != last)
    {
        // 同步标志位，防止连续重置
        last = current;
        // 遍历并重置所有10个滤波器
        for (i = 0; i < 10; i++)
        {
            // 使用当前的测量值作为所有滤波器的初始状态值
            Kalman_init(&state[i], z_measure, 0.01);
        }
    }
    // 如果没有触发重置，则正常执行滤波
    else
    {
        // 对索引为 i 的滤波器应用卡尔曼滤波
        z_measure = kalman_filter(&state[i], z_measure);
    }
    return z_measure;
}

// 定义全局变量，用于控制 `kalman_thd` 函数的重置逻辑
uint8_t last1 = 1, current1 = 0;
// kalman_thd使用的独立滤波器状态。
kalman_state state1;

/**
 * @brief  kalman_thd单数据流滤波封装
 * @details 此函数管理一个独立的滤波器 `state1`，并同样包含一个外部触发的重置机制。
 * @param  z_measure  当前的测量值。
 * @retval (float) 滤波后的值或初始值。
 */
float kalman_thd(float z_measure)
{
    // 检查重置标志
    if (current1 != last1)
    {
        // 同步标志位
        last1 = current1;
        // 使用当前测量值作为初始状态，重置滤波器
        Kalman_init(&state1, z_measure, 1);
    }
    else
    {
        // 正常执行滤波
        z_measure = kalman_filter(&state1, z_measure);
    }
    return z_measure;
}
