/*******************************************************************************
 * @file      my_hmi.c
 * @author    左岚
 * @version   V2.0
 * @date      2025-07-18
 * @brief     串口HMI ASCII指令驱动
 * @note      每条指令以`\xff\xff\xff`结束，UART句柄按值传入现有接口。
 *******************************************************************************/

#include "my_hmi.h"
#include "math.h" // HMI_Send_Float使用pow()。

/**
 * @brief  向HMI控件发送字符串数据
 * @param  huartr_hmi  用于通信的UART句柄
 * @param  obj_name    控件的名称 (例如 "p0.t0")
 * @param  show_data   要显示的字符串内容
 * @retval None
 */
void HMI_Send_String(UART_HandleTypeDef huartr_hmi, const char *obj_name,
                     const char *show_data)
{
    UART_HandleTypeDef huart_hmi = huartr_hmi;
    // 指令格式: 控件名.txt="字符串" + 结束符
    my_printf(&huart_hmi, "%s.txt=\"%s\"\xff\xff\xff", obj_name, show_data);
}

/**
 * @brief  向HMI控件发送整数数据
 * @param  huart_hmi   用于通信的UART句柄
 * @param  obj_name    控件的名称 (例如 "p0.n0")
 * @param  show_data   要显示的整数值
 * @retval None
 */
void HMI_Send_Int(UART_HandleTypeDef huartr_hmi, const char *obj_name,
                  int show_data)
{
    UART_HandleTypeDef huart_hmi = huartr_hmi;
    // 指令格式: 控件名.val=整数值 + 结束符
    my_printf(&huart_hmi, "%s.val=%d\xff\xff\xff", obj_name, show_data);
}

/**
 * @brief  向HMI控件发送浮点数数据
 * @details 通过将浮点数放大为整数来间接实现浮点数的显示。
 * @param  huart_hmi   用于通信的UART句柄
 * @param  obj_name    控件的名称 (例如 "p0.n0")
 * @param  show_data   要显示的浮点数值
 * @param  point_index 小数点后保留的位数
 * @retval None
 */
void HMI_Send_Float(UART_HandleTypeDef huartr_hmi, const char *obj_name,
                    float show_data, int point_index)
{
    UART_HandleTypeDef huart_hmi = huartr_hmi;
    // 例如: 发送12.34, point_index=2, 则temp = 12.34 * 10^2 = 1234。
    // HMI端需要相应配置小数点位置才能正确显示。
    int temp = show_data * pow(10, point_index);
    my_printf(&huart_hmi, "%s.val=%d\xff\xff\xff", obj_name, temp);
}

/**
 * @brief  清除指定波形控件上的某条通道数据
 * @param  huart_hmi   用于通信的UART句柄
 * @param  obj_name    波形控件的名称 (例如 "s0")
 * @param  ch          要清除的波形通道编号 (通常从0开始)
 * @retval None
 */
void HMI_Wave_Clear(UART_HandleTypeDef huartr_hmi, const char *obj_name,
                    int ch)
{
    UART_HandleTypeDef huart_hmi = huartr_hmi;
    // 指令格式: cle 控件名,通道号 + 结束符
    my_printf(&huart_hmi, "cle %s.id,%d\xff\xff\xff", obj_name, ch);
    HAL_Delay(5);
}

/**
 * @brief  向指定波形控件添加单个数据点 (低速)
 * @param  huart_hmi   用于通信的UART句柄
 * @param  obj_name    波形控件的名称 (例如 "s0")
 * @param  ch          波形通道编号
 * @param  val         数据值 (范围0-255)
 * @retval None
 */
void HMI_Write_Wave_Low(UART_HandleTypeDef huartr_hmi, const char *obj_name,
                        int ch, int val)
{
    UART_HandleTypeDef huart_hmi = huartr_hmi;
    // 指令格式: add 控件ID,通道号,数值 + 结束符
    my_printf(&huart_hmi, "add %s.id,%d,%d\xff\xff\xff", obj_name, ch, val);
}

/**
 * @brief  使用透明传输模式向波形控件快速发送一批数据 (高速)
 * @param  huart_hmi   用于通信的UART句柄
 * @param  obj_name    波形控件的名称 (例如 "s0")
 * @param  ch          波形通道编号
 * @param  len         要发送的数据点数量
 * @param  val         存储数据点的数组指针 (每个值范围0-255)
 * @retval None
 */
void HMI_Write_Wave_Fast(UART_HandleTypeDef huartr_hmi, const char *obj_name,
                         int ch, uint16_t len, const uint8_t *val)
{
    UART_HandleTypeDef huart_hmi = huartr_hmi;
    my_printf(&huart_hmi, "addt %s.id,%d,%d\xff\xff\xff", obj_name, ch, len);
    HAL_Delay(20);
    HAL_UART_Transmit(&huart_hmi, (uint8_t *)val, len, 100U);
}
