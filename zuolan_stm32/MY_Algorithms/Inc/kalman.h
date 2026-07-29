/*******************************************************************************
 * @file      kalman.h
 * @author    左岚
 * @version   V1.0
 * @date      2025-07-18
 * @brief     一维卡尔曼滤波器的数据结构与函数声明
 * @note      此文件定义了卡尔曼滤波器所需的核心数据结构`kalman_state`，
 * 并声明了滤波器的初始化、核心计算以及两个应用层封装函数的外部接口。
 *******************************************************************************/

#ifndef __KALMAN_H
#define __KALMAN_H

#include <stdint.h>

/**
 * @brief 卡尔曼滤波器状态与参数的结构体定义
 * @note  该结构体聚合了运行一个一维卡尔曼滤波器所需的所有状态变量和模型参数。
 */
typedef struct
{
    float x;    // 状态变量 (后验估计值) x(k|k)，即滤波器的输出。
    float A;    // 状态转移矩阵 A，在kalman.c中被设为1，代表恒定值模型。
    float H;    // 观测矩阵 H，在kalman.c中被设为1，代表直接观测。
    float Q;    // 过程噪声协方差 Q，反映了模型预测的不确定性。
    float R;    // 测量噪声协方差 R，反映了测量值本身的不确定性。
    float p;    // 误差协方差 (后验估计) p(k|k)，反映了当前状态估计值的不确定性。
    float k;    // 卡尔曼增益 K，决定了在更新状态时，预测值与测量值的权重。
} kalman_state;


/*******************************************************************************
 * 外部函数声明
 *******************************************************************************/

/**
 * @brief  卡尔曼滤波器初始化函数声明
 * @see    kalman.c 文件中的具体实现
 */
void Kalman_init(kalman_state *state, float init_x, float init_p);

/**
 * @brief  卡尔曼滤波器核心计算函数声明
 * @see    kalman.c 文件中的具体实现
 */
float kalman_filter(kalman_state *state, float z_measure);

/**
 * @brief  针对滤波器组的应用封装函数声明
 * @see    kalman.c 文件中的具体实现
 */
float kalman(uint8_t i, float z_measure);

/**
 * @brief  kalman_thd单数据流滤波封装
 * @see    kalman.c 文件中的具体实现
 */
float kalman_thd(float z_measure);

#endif /* __KALMAN_H */
