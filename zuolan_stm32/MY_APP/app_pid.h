/*******************************************************************************
 * @file      app_pid.h
 * @author    左岚
 * @version   V1.1
 * @date      2025-07-18
 * @brief     PID控制器相关的宏定义、数据结构和函数声明
 * @note      此头文件定义了PID控制器的核心配置。通过宏 `INCR_LOCT_SELECT`
 * 可以选择编译增量式或位置式PID，并为两种模式分别提供了独立的
 * P, I, D参数和采样周期配置。同时，定义了PID控制器所需的数据
 * 结构 `PID_TypeDef` 并声明了相关的外部接口函数。
 *******************************************************************************/

#ifndef APP_APP_PID_H_
#define APP_APP_PID_H_

// 包含板级支持包(BSP)的系统头文件，通常会提供一些底层硬件相关的定义，
// 例如本文件中使用的 __IO 宏。
#include "bsp_system.h"

/*******************************************************************************
 * PID 参数与模式配置
 *******************************************************************************/

// PID算法选择宏： 0 代表 位置式PID，1 代表 增量式PID。
// 整个工程将根据此宏的值来决定编译哪一种PID算法及其对应的参数。
#define INCR_LOCT_SELECT 1

#if INCR_LOCT_SELECT
/* =========== 增量式PID参数相关宏 =========== */
// 增量式PID的优点是控制增量，系统发生大的跳变时，影响较小，且没有积分饱和问题。
#define KP                  0.8f    // 比例(P)增益 (Proportional Gain)
#define KI                  0.7f    // 积分(I)增益 (Integral Gain)
#define KD                  0.20f   // 微分(D)增益 (Derivative Gain)
#define SMAPLSE_PID_SPEED   1       // PID计算的采样周期，单位：毫秒(ms)

#else
/* =========== 位置式PID参数相关宏 =========== */
// 位置式PID输出的是控制量的绝对值，直观易懂，适用于需要直接控制执行器位置的场合。
#define KP                  10.0f   // 比例(P)增益
#define KI                  0.5f    // 积分(I)增益
#define KD                  6.00f   // 微分(D)增益
#define SMAPLSE_PID_SPEED   10      // PID计算的采样周期，单位：毫秒(ms)
#endif


/*******************************************************************************
 * PID 数据结构定义
 *******************************************************************************/

/**
 * @brief PID参数与状态的结构体定义
 * @note  该结构体聚合了PID控制器运行所需的所有变量，包括设定值、状态值、
 * 误差历史以及PID增益系数。
 * `__IO` 是一个易失性限定符(volatile)，用于告知编译器不要对这些变量的
 * 读写操作进行优化，确保每次都从内存中真实读写，在嵌入式系统中很常用。
 */
typedef struct
{
    __IO float SetPoint;        // 设定目标 (Desired Value)
    __IO float ActualValue;     // 控制器计算出的期望输出值 (Manipulated Value)
    __IO float SumError;        // 误差累加值（主要用于位置式PID的积分项）
    
    __IO float Proportion;      // 比例增益 (P)
    __IO float Integral;        // 积分增益 (I)
    __IO float Derivative;      // 微分增益 (D)

    __IO float Error;           // 当前误差 e(k) = SetPoint - FeedbackValue
    __IO float LastError;       // 上一次的误差 e(k-1)
    __IO float PrevError;       // 上上次的误差 e(k-2)（主要用于增量式PID的微分项）
} PID_TypeDef;


/*******************************************************************************
 * 外部函数声明
 *******************************************************************************/

/**
 * @brief PID控制主处理函数声明
 * @see   app_pid.c 文件中的具体实现
 */
void Pid_Proc(void);

/**
 * @brief PID控制器初始化函数声明
 * @see   app_pid.c 文件中的具体实现
 */
void PID_Init(void);


#endif /* APP_APP_PID_H_ */
