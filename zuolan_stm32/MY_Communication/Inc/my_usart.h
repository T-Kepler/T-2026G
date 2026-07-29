/*******************************************************************************
 * @file      my_usart.h
 * @author    左岚
 * @version   V1.0
 * @date      2025-07-18
 * @brief     该文件是 my_usart.c 的头文件。
 * @note      此文件定义了自定义串口驱动所需的宏、外部变量声明和函数原型。
 * 包含了对多个串口(USART1, 2, 3)共用的缓冲区大小定义，以及
 * 供其他模块调用的全局变量和函数。
 *******************************************************************************/

#ifndef __MY_USART_H__
#define __MY_USART_H__

#include "stm32f4xx_hal.h" // 包含STM32F4系列的HAL库头文件
#include "usart.h"         // 包含CubeMX生成的usart初始化相关头文件
#include "stdint.h"        // 包含标准整型定义

// --- 宏定义 ---

#define RX_BUFFER_SIZE 128  ///< 定义每个串口接收缓冲区的统一大小，单位为字节
#define MIN_FRAME_SIZE 3    ///< 定义一个有效数据帧的最小长度（例如：帧头+数据+帧尾）

// --- 外部变量声明 ---
// 这些变量在 my_usart.c 中定义，此处声明以便在工程的其他文件中使用。

// 串口中断接收临时存储变量
extern uint8_t rxTemp1, rxTemp2, rxTemp3;

// 各串口接收数据的主缓冲区
extern uint8_t rxBuffer1[RX_BUFFER_SIZE], rxBuffer2[RX_BUFFER_SIZE], rxBuffer3[RX_BUFFER_SIZE];

// 各串口接收缓冲区当前写入位置的索引
extern uint16_t rxIndex1, rxIndex2, rxIndex3;

// 串口接收到完整指令的标志位 (volatile 关键字防止编译器进行不当优化)
extern volatile uint8_t commandReceived1, commandReceived3;


// --- 函数原型声明 ---

/**
 * @brief  自定义的printf函数，可将格式化字符串通过指定的UART端口发送。
 * @param  huart:  目标串口的句柄指针。
 * @param  format: C语言格式化字符串。
 * @param  ...:    可变参数列表。
 * @retval int:    成功发送的字节数。
 */
int my_printf(UART_HandleTypeDef *huart, const char *format, ...);

/**
 * @brief  HAL库UART接收完成中断的公共回调函数。
 * @note   此函数是一个弱定义函数，在 my_usart.c 中被重写以实现自定义的接收逻辑。
 * 用户不应直接调用此函数。
 * @param  huart: 触发中断的串口句柄指针。
 * @retval None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

#endif // __MY_USART_H__
