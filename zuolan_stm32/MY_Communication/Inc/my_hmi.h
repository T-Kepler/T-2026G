/*******************************************************************************
 * @file      my_hmi.h
 * @author    左岚
 * @version   V2.0
 * @date      2025-07-18
 * @brief     串口HMI ASCII指令接口
 *******************************************************************************/

#ifndef __MY_HMI_H__
#define __MY_HMI_H__

#include "my_usart.h" // 包含自定义串口库头文件,以获取my_printf和UART_HandleTypeDef的定义

// --------------------------- 外部函数声明 ---------------------------

/**
 * @brief  向HMI发送字符串的函数声明
 * @param  huartr_hmi (应为 huart_hmi) 用于通信的UART句柄 (例如 &huart1)
 * @param  obj_name   控件的名称
 * @param  show_data  要显示的字符串
 */
void HMI_Send_String(UART_HandleTypeDef huartr_hmi, const char *obj_name,
                     const char *show_data);

/**
 * @brief  向HMI发送整数的函数声明
 * @param  huartr_hmi (应为 huart_hmi) 用于通信的UART句柄
 * @param  obj_name   控件的名称
 * @param  show_data  要显示的整数值
 */
void HMI_Send_Int(UART_HandleTypeDef huartr_hmi, const char *obj_name,
                  int show_data);

/**
 * @brief  向HMI发送浮点数的函数声明
 * @param  huartr_hmi (应为 huart_hmi) 用于通信的UART句柄
 * @param  obj_name   控件的名称
 * @param  show_data  要显示的浮点数值
 * @param  point_index 小数点后的位数
 */
void HMI_Send_Float(UART_HandleTypeDef huartr_hmi, const char *obj_name,
                    float show_data, int point_index);

/**
 * @brief  清除HMI波形通道的函数声明
 * @param  huartr_hmi (应为 huart_hmi) 用于通信的UART句柄
 * @param  obj_name   波形控件的名称
 * @param  ch         通道编号
 */
void HMI_Wave_Clear(UART_HandleTypeDef huartr_hmi, const char *obj_name,
                    int ch);

/**
 * @brief  向HMI波形通道逐点写入数据的函数声明 (低速)
 * @param  huartr_hmi (应为 huart_hmi) 用于通信的UART句柄
 * @param  obj_name   波形控件的名称
 * @param  ch         通道编号
 * @param  val        数据点的值 (0-255)
 */
void HMI_Write_Wave_Low(UART_HandleTypeDef huartr_hmi, const char *obj_name,
                        int ch, int val);

/**
 * @brief  向HMI波形通道快速写入批量数据的函数声明 (高速)
 * @param  huartr_hmi (应为 huart_hmi) 用于通信的UART句柄
 * @param  obj_name   波形控件的名称
 * @param  ch         通道编号
 * @param  len        数据点的数量
 * @param  val        指向数据点数组的指针
 */
void HMI_Write_Wave_Fast(UART_HandleTypeDef huartr_hmi, const char *obj_name,
                         int ch, uint16_t len, const uint8_t *val);

#endif // __MY_HMI_H__
