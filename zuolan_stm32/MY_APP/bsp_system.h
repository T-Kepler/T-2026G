/*******************************************************************************
 * @file      bsp_system.h
 * @author    左岚
 * @version   V1.1
 * @date      2025-07-18
 * @brief     板级支持包(BSP)系统级头文件
 * @note      汇总公共依赖并声明跨模块共享变量。
 *******************************************************************************/

#ifndef BSP_SYSTEM_H
#define BSP_SYSTEM_H

/* STM32 HAL和C标准库 */
#include "main.h"         // STM32CubeMX生成的主头文件，包含引脚定义等
#include "stm32f4xx_hal.h"// STM32F4系列的HAL库核心文件
#include "stdio.h"        // 标准输入输出库
#include "string.h"       // 字符串处理库
#include "stdarg.h"       // 可变参数宏

/* 板载外设驱动 (由STM32CubeMX生成或手动编写) */
#include "dac.h"          // 数模转换器(DAC)驱动
#include "usart.h"        // 通用同步/异步收发器(USART)驱动
#include "gpio.h"         // 通用输入输出(GPIO)驱动
#include "fmc.h"          // FMC外部总线

/* 中间件与算法库 */
#include "arm_math.h"     // ARM官方的CMSIS-DSP数学计算库
#include "scheduler.h"    // 非抢占式任务调度器
#include "my_fft.h"       // 自定义的快速傅里叶变换(FFT)功能模块
#include "my_filter.h"    // 自定义的数字滤波器模块
#include "kalman.h"       // 卡尔曼滤波器模块

/* 自定义功能与应用层模块 */
#include "ad_measure.h"     // AD采样测量模块
#include "freq_measure.h"   // 频率测量模块
#include "phase_measure.h"  // 相位测量模块
#include "my_usart.h"       // 自定义的串口通信协议处理模块
#include "my_usart_pack.h"  // 自定义的串口数据打包/解包模块
#include "da_output.h"      // DA输出控制模块
#include "AD9959.h"         // AD9959 DDS芯片的驱动模块
#include "commond_init.h"   // 项目公共类型、控制位和FMC映射
#include "app_pid.h"        // PID控制器应用模块
#include "key_app.h"        // 按键应用处理模块
#include "AD9833.h"
/* 跨模块共享变量，名称为兼容现有接口而保留。 */

extern u32 Modulated_wave;      // 用于存储调制波数据的变量
extern u8 mdoe_flag;            // 系统模式标志
extern u16 Phase;               // 存储当前系统的相位值
extern u32 val;                 // 共享数据值
extern u8 Carrier_indx;         // 载波索引
extern int32_t output;          // PID限幅后的输出值
extern u32 pid_vin;             // PID控制器的输入反馈值
extern float detected_freq;     // 检测频率

#endif /* BSP_SYSTEM_H */
