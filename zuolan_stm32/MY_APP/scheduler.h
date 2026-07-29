/*******************************************************************************
 * @file      scheduler.h
 * @author    左岚
 * @version   V1.0
 * @date      2025-07-18
 * @brief     非抢占式任务调度器接口
 *******************************************************************************/

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "bsp_system.h"

/** @brief 初始化静态任务表。 */
void scheduler_init(void);

/** @brief 在主循环中轮询到期任务。 */
void scheduler_run(void);

#endif /* SCHEDULER_H */
