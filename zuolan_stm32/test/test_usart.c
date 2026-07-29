/* 串口接收板级示例。HAL回调由中断自动触发，应用层不要直接调用。 */
#include "my_usart.h"
#include <string.h>

void example_usart_start_receive(void)
{
    HAL_UART_Receive_IT(&huart1, &rxTemp1, 1);
    HAL_UART_Receive_IT(&huart2, &rxTemp2, 1);
    HAL_UART_Receive_IT(&huart3, &rxTemp3, 1);
    my_printf(&huart1, "串口接收已启动\r\n");
}

void example_usart_poll(void)
{
    if (commandReceived1)
    {
        commandReceived1 = 0;
        my_printf(&huart1, "USART1: %s\r\n", rxBuffer1);
        memset(rxBuffer1, 0, sizeof(rxBuffer1));
    }

    if (commandReceived3)
    {
        commandReceived3 = 0;
        my_printf(&huart1, "USART3: %s\r\n", rxBuffer3);
        memset(rxBuffer3, 0, sizeof(rxBuffer3));
    }
}
