/*******************************************************************************
 * @file      scheduler.c
 * @author    左岚
 * @version   V1.0
 * @date      2025-07-18
 * @brief     基于HAL毫秒时基的非抢占式任务调度器
 *******************************************************************************/

#include "scheduler.h"
#include "key_app.h"
#include "g_signal_app.h"

// 任务数量在scheduler_init中由静态任务表计算。
uint8_t task_num;

/**
 * @brief 任务控制块结构体定义
 * @note  每个任务都由一个该结构体来描述。
 */
typedef struct
{
    void (*task_func)(void);
    uint32_t rate_ms;
    uint32_t last_run;
} task_t;

/**
 * @brief  当前启用的静态任务表
 */
static task_t scheduler_task[] =
    {
        {key_proc, 10, 0},
        {GSignalApp_Task, 10, 0},
};

/**
 * @brief  调度器初始化函数
 * @details 必须在首次调用`scheduler_run()`前执行。
 * @param  None
 * @retval None
 */
void scheduler_init(void)
{
    task_num = sizeof(scheduler_task) / sizeof(task_t);
}

/**
 * @brief  调度器主运行函数
 * @details 在主循环中轮询任务表；无补跑机制，任务执行时间会影响后续任务。
 * @param  None
 * @retval None
 */
void scheduler_run(void)
{
    for (uint8_t i = 0; i < task_num; i++)
    {
        uint32_t now_time = HAL_GetTick();

        // 无符号差值兼容HAL_GetTick回绕。
        if (now_time - scheduler_task[i].last_run >= scheduler_task[i].rate_ms)
        {
            scheduler_task[i].last_run = now_time;
            scheduler_task[i].task_func();
        }
    }
}
