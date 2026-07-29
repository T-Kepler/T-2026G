/*******************************************************************************
 * @file      app_pid.c
 * @author    左岚
 * @version   V1.1
 * @date      2025-07-18
 * @brief     PID控制器功能实现
 * @note      `INCR_LOCT_SELECT`在编译期选择增量式或位置式PID。
 *******************************************************************************/

#include "app_pid.h"

PID_TypeDef PID;

/**
 * @brief     初始化PID控制器
 * @details   该函数用于设置PID控制器的初始状态和增益系数。
 * 在PID控制器开始运行前，必须调用此函数进行一次初始化。
 * @param     None
 * @retval    None
 */
void PID_Init(void)
{
    PID.SetPoint = 3.0f;
    PID.ActualValue = 0.0f;     // PID控制器计算出的输出值，初始化为0。
    PID.SumError = 0.0f;        // 累积误差，主要用于位置式PID的积分项，初始化为0。

    // 增益由app_pid.h中的编译模式选择。
    PID.Proportion = KP;        // 比例(Proportional)增益 (Kp)
    PID.Integral = KI;          // 积分(Integral)增益 (Ki)
    PID.Derivative = KD;        // 微分(Derivative)增益 (Kd)
    
    // 初始化误差相关的变量，防止第一次计算时使用未定义的历史值。
    PID.Error = 0.0f;           // 当前误差 e(k)
    PID.LastError = 0.0f;       // 上一次的误差 e(k-1)
    PID.PrevError = 0.0f;       // 上上次的误差 e(k-2), 主要用于增量式PID的微分项。
}

/**
 * @brief     PID闭环控制核心计算函数
 * @details   根据选择的PID算法（增量式或位置式），计算控制器的输出。
 * 增量式PID输出的是控制量的变化量(Δu)，优点是无积分饱和问题，且误动作影响小。
 * 位置式PID输出的是控制量的绝对值(u)，适用于需要直接知道控制位置的系统。
 * @param     *PID 指向PID结构体实例的指针，包含了控制器所有状态和参数。
 * @param     Feedback_value 从传感器或ADC等获取的当前实际测量值（反馈值）。
 * @retval    (int32_t) 经过PID计算后，控制器期望的输出值。
 */
int32_t increment_pid_ctrl(PID_TypeDef *PID, float Feedback_value)
{
    /* 步骤1: 计算当前误差 */
    // 误差 = 目标值 - 实际反馈值
    PID->Error = (float)(PID->SetPoint - Feedback_value);

    // INCR_LOCT_SELECT=1编译增量式，否则编译位置式。
#if INCR_LOCT_SELECT /* 编译此部分代码时，使用增量式PID算法 */

    /* 增量式PID核心公式: Δu(k) = Kp[e(k)-e(k-1)] + Ki[e(k)] + Kd[e(k)-2e(k-1)+e(k-2)] */
    /* 最终输出: u(k) = u(k-1) + Δu(k) */
    
    // 计算增量 Δu(k) 并累加到上一次的输出值上
    PID->ActualValue += 
        // 比例环节 P: Kp * (e(k) - e(k-1))
        (PID->Proportion * (PID->Error - PID->LastError))
        // 积分环节 I: Ki * e(k)
        + (PID->Integral * PID->Error)
        // 微分环节 D: Kd * ((e(k) - e(k-1)) - (e(k-1) - e(k-2))) = Kd * (e(k) - 2*e(k-1) + e(k-2))
        + (PID->Derivative * (PID->Error - 2 * PID->LastError + PID->PrevError));

    /* 步骤2: 更新历史误差，为下一次迭代做准备 */
    PID->PrevError = PID->LastError; // 将上一次的误差存为上上次的误差 e(k-2) = e(k-1)
    PID->LastError = PID->Error;     // 将当前误差存为上一次的误差   e(k-1) = e(k)

#else /* 编译此部分代码时，使用位置式PID算法 */

    /* 位置式PID核心公式: u(k) = Kp*e(k) + Ki*Σe(i) + Kd*[e(k)-e(k-1)] */
    
    /* 步骤2: 累加误差，用于积分项 */
    PID->SumError += PID->Error;
    // (可选) 在这里可以加入积分限幅，防止积分饱和 (e.g., if(PID->SumError > MAX_SUM_ERR) PID->SumError = MAX_SUM_ERR;)

    /* 计算绝对输出值 u(k) */
    PID->ActualValue = 
        // 比例环节 P: Kp * e(k)
        (PID->Proportion * PID->Error)
        // 积分环节 I: Ki * Σe(i)
        + (PID->Integral * PID->SumError)
        // 微分环节 D: Kd * (e(k) - e(k-1))
        + (PID->Derivative * (PID->Error - PID->LastError));
        
    /* 步骤3: 更新历史误差，为下一次迭代做准备 */
    PID->LastError = PID->Error; // 将当前误差存为上一次的误差 e(k-1) = e(k)

#endif
    /* 步骤3/4: 返回经过PID计算后的输出值 */
    // 将浮点计算结果转换为整型返回
    return ((int32_t)(PID->ActualValue));
}


// 定义全局变量用于存储PID的输出和输入
int32_t output = 38; // 输出下限与初始值
float vin;           // 从外部获取并经过处理的反馈电压值

/**
 * @brief     PID控制主处理函数
 * @details   此函数封装了PID控制的完整流程：获取并处理反馈值、
 * 调用PID核心函数进行计算、对输出结果进行限幅处理，
 * 最后将结果赋给实际的控制变量。通常在定时器中断或主循环中周期性调用。
 * @param     None
 * @retval    None
 */
void Pid_Proc(void)
{
    /* 步骤1: 获取并处理反馈值 */
    // 将AD2峰峰值换算到当前控制反馈量纲。
    vin = vol_amp2 / 2 * 10;
    
    /* 步骤2: 调用PID控制器计算输出值 */
    // 将PID结构体和处理后的反馈值传入核心计算函数
    output = increment_pid_ctrl(&PID, vin);
    
    /* 步骤3: 输出限幅 (Clamping / Saturation) */
    // 当前执行输出范围固定为38～1023。
    if (output > 1023)
    {
        output = 1023;
    }
    else if (output < 38)
    {
        output = 38;
    }
    
    /* 步骤4: 将最终值赋给执行变量 */
    pid_vin = output;
}
