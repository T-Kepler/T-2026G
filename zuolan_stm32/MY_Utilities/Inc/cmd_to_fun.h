/**
 * @file cmd_to_fun.h
 * @brief FPGA控制命令到底层函数映射的头文件
 * @details
 * 该文件为硬件抽象层（HAL）提供了公共接口，声明了一系列用于直接操控FPGA
 * 控制寄存器的底层函数。这些函数被上层应用模块调用，以控制FPGA的各项功能。
 *
 * @author 左岚
 * @date 2025-07-18
 */
#ifndef __CMD_TO_FUN_H__
#define __CMD_TO_FUN_H__

#include "commond_init.h"

// *********************************************************************************
// 外部函数声明
// *********************************************************************************

/**
 * @brief 初始化FPGA主控制寄存器。
 */
void CTRL_INIT(void);

/**
 * @brief 启动FPGA的DA波形发生器。
 */
void DA_FPGA_START(void);

/**
 * @brief 停止FPGA的DA波形发生器。
 */
void DA_FPGA_STOP(void);

/**
 * @brief 使能AD测频计数器清零（即复位计数器）。
 * @param ch 通道号 (1: AD1, 2: AD2)
 */
void AD_FREQ_CLR_ENABLE(int ch);

/**
 * @brief 禁能AD测频计数器清零（即释放复位，使其可工作）。
 * @param ch 通道号 (1: AD1, 2: AD2)
 */
void AD_FREQ_CLR_DISABLE(int ch);

/**
 * @brief 启动AD频率测量。
 * @param ch 通道号 (1: AD1, 2: AD2)
 */
void AD_FREQ_START(int ch);

/**
 * @brief 停止AD频率测量。
 * @param ch 通道号 (1: AD1, 2: AD2)
 */
void AD_FREQ_STOP(int ch);

/**
 * @brief 使能AD采样时钟生成。
 * @param ch 通道号 (1: AD1, 2: AD2, 3: 同时使能两个通道)
 */
void AD_FREQ_SET(int ch);

/**
 * @brief 使能AD数据写入FIFO。
 * @param ch 通道号 (1: AD1, 2: AD2, 3: 同时使能两个通道)
 */
void AD_FIFO_WRITE_ENABLE(int ch);

/**
 * @brief 禁能AD数据写入FIFO。
 * @param ch 通道号 (1: AD1, 2: AD2)
 */
void AD_FIFO_WRITE_DISABLE(int ch);

/**
 * @brief 使能从FIFO读取AD数据。
 * @param ch 通道号 (1: AD1, 2: AD2)
 */
void AD_FIFO_READ_ENABLE(int ch);

/**
 * @brief 禁能从FIFO读取AD数据。
 * @param ch 通道号 (1: AD1, 2: AD2)
 */
void AD_FIFO_READ_DISABLE(int ch);

#endif //__CMD_TO_FUN_H__
