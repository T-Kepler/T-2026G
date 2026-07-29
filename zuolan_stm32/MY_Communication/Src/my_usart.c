/**
 * @file my_usart.c
 * @author 左岚
 * @brief USART接收缓冲与格式化输出
 */
#include "my_usart.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "my_usart_pack.h"
#include "g_signal_app.h"
// 串口1的缓冲区和状态变量
uint8_t rxBuffer1[RX_BUFFER_SIZE];     ///< 串口1接收缓冲区
uint8_t rxTemp1;                       ///< 串口1中断接收临时变量
uint16_t rxIndex1 = 0;                 ///< 串口1当前接收缓冲区索引
volatile uint8_t commandReceived1 = 0; ///< 串口1接收到命令标志

// 串口3的缓冲区和状态变量
uint8_t rxBuffer3[RX_BUFFER_SIZE];     ///< 串口3接收缓冲区
uint8_t rxTemp3;                       ///< 串口3中断接收临时变量
uint16_t rxIndex3 = 0;                 ///< 串口3当前接收缓冲区索引
volatile uint8_t commandReceived3 = 0; ///< 串口3接收到命令标志

// 串口2数据包缓冲区和变量
uint8_t rxBuffer2[RX_BUFFER_SIZE]; ///< 串口2接收缓冲区
uint8_t rxTemp2;                   ///< 串口2中断接收临时变量
uint16_t rxIndex2 = 0;             ///< 串口2当前接收缓冲区索引
volatile uint8_t frameStarted = 0; ///< 帧开始标志
static uint8_t hmiSkipTouchBytes = 0;
static uint8_t hmiPageResponseBytes = 0U;
static uint8_t hmiPageResponseId = 0xFFU;
static uint8_t hmiPageResponseValid = 0U;
static uint8_t hmiLastPageId = 0xFFU;

static void HMI_HandlePageId(uint8_t page_id)
{
    if (page_id == 0U)
        GSignalApp_HandleControl(G_SIGNAL_CONTROL_RESULT_PAGE_READY);
    else if (page_id == 1U)
        GSignalApp_HandleControl(G_SIGNAL_CONTROL_WAVE_PAGE_READY);
    else
        return;
    hmiLastPageId = page_id;
}

static uint8_t HMI_HandleRawByte(uint8_t byte)
{
    if (hmiSkipTouchBytes > 0U)
    {
        hmiSkipTouchBytes--;
        return 1U;
    }

    if (hmiPageResponseBytes > 0U)
    {
        if (hmiPageResponseBytes == 4U)
            hmiPageResponseId = byte;
        else if (byte != 0xFFU)
            hmiPageResponseValid = 0U;

        hmiPageResponseBytes--;
        if (hmiPageResponseBytes == 0U && hmiPageResponseValid &&
            hmiPageResponseId != hmiLastPageId)
            HMI_HandlePageId(hmiPageResponseId);
        return 1U;
    }

    switch (byte)
    {
    case 0x31:
        GSignalApp_HandleControl(G_SIGNAL_CONTROL_MODE_NEXT);
        return 1U;
    case 0x32:
        GSignalApp_HandleControl(G_SIGNAL_CONTROL_PERIOD_TOGGLE);
        return 1U;
    case 0x33:
        GSignalApp_HandleControl(G_SIGNAL_CONTROL_VIEW_TOGGLE);
        return 1U;
    case 0x34:
        GSignalApp_HandleControl(G_SIGNAL_CONTROL_MEASURE);
        return 1U;
    case 0x35:
        HMI_HandlePageId(0U);
        return 1U;
    case 0x36:
        HMI_HandlePageId(1U);
        return 1U;
    case 0x37:
        GSignalApp_HandleControl(G_SIGNAL_CONTROL_CLEAR);
        return 1U;
    case 0x65:
        hmiSkipTouchBytes = 6U;
        return 1U;
    case 0x66:
        hmiPageResponseBytes = 4U;
        hmiPageResponseValid = 1U;
        return 1U;
    default:
        return 0U;
    }
}

/**
 * @brief 格式化打印并通过指定串口发送
 * @param huart 串口句柄
 * @param format 格式化字符串
 * @param ... 可变参数
 * @return 返回发送的字节数
 */
int my_printf(UART_HandleTypeDef *huart, const char *format, ...)
{
    char buffer[512]; // 临时缓冲区，用于存储格式化后的字符串
    va_list arg;      // 可变参数列表
    int len;

    va_start(arg, format);                                // 初始化可变参数列表
    len = vsnprintf(buffer, sizeof(buffer), format, arg); // 格式化字符串
    va_end(arg);                                          // 清理可变参数列表

    if (len < 0)
        return len;
    if ((size_t)len >= sizeof(buffer))
        len = sizeof(buffer) - 1U;
    HAL_UART_Transmit(huart, (uint8_t *)buffer, (uint16_t)len, 0xFF); // 发送格式化后的字符串
    return len;
}

/**
 * @brief 串口接收中断回调函数
 * @param huart 串口句柄
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	
    if (huart->Instance == USART1) // USART1的接收中断
    {
        if (rxIndex1 < RX_BUFFER_SIZE - 1)
        {
            // 如果接收到换行符并且前一个字符是回车符，标记命令接收完成
            if (rxTemp1 == '\n' && rxIndex1 > 0U &&
                rxBuffer1[rxIndex1 - 1U] == '\r')
            {
                commandReceived1 = 1;           // 设置命令接收标志
                rxBuffer1[rxIndex1 - 1] = '\0'; // 添加字符串结束符
                rxIndex1 = 0;                   // 重置接收索引
            }
            else
            {
                rxBuffer1[rxIndex1++] = rxTemp1; // 保存接收到的数据
            }
        }
        else
        {
            rxIndex1 = 0; // 缓冲区溢出，重置索引
        }
        HAL_UART_Receive_IT(&huart1, &rxTemp1, 1); // 再次启动接收中断
    }

    if (huart->Instance == USART3) // USART3的接收中断
    {
        if (rxIndex3 < RX_BUFFER_SIZE - 1)
        {
            // 如果接收到换行符并且前一个字符是回车符，标记命令接收完成
            if (rxTemp3 == '\n' && rxIndex3 > 0U &&
                rxBuffer3[rxIndex3 - 1U] == '\r')
            {
                commandReceived3 = 1;           // 设置命令接收标志
                rxBuffer3[rxIndex3 - 1] = '\0'; // 添加字符串结束符
                rxIndex3 = 0;                   // 重置接收索引
            }
            else
            {
                rxBuffer3[rxIndex3++] = rxTemp3; // 保存接收到的数据
            }
        }
        else
        {
            rxIndex3 = 0; // 缓冲区溢出，重置索引
        }
        HAL_UART_Receive_IT(&huart3, &rxTemp3, 1); // 再次启动接收中断
    }

    if (huart->Instance == USART2) // USART2的接收中断
    {
        if (!frameStarted && HMI_HandleRawByte(rxTemp2))
        {
            HAL_UART_Receive_IT(&huart2, &rxTemp2, 1);
            return;
        }

        if (rxTemp2 == FRAME_HEADER) // 检测到帧头
        {
            frameStarted = 1;                // 标记帧开始
            rxIndex2 = 0;                    // 重置缓冲区索引
            rxBuffer2[rxIndex2++] = rxTemp2; // 保存帧头
        }
        else if (frameStarted) // 如果已经接收到帧头
        {
            rxBuffer2[rxIndex2++] = rxTemp2; // 保存接收到的数据

            if (rxTemp2 == FRAME_TAIL) // 检测到帧尾
            {
                frameStarted = 0;                // 清除帧开始标志
                ParseFrame(rxBuffer2, rxIndex2); // 调用解析函数处理数据
                rxIndex2 = 0;                    // 重置缓冲区索引
            }

            // 防止缓冲区溢出
            if (rxIndex2 >= RX_BUFFER_SIZE)
            {
                frameStarted = 0;                     // 重置帧开始标志
                rxIndex2 = 0;                         // 重置缓冲区索引
                memset(rxBuffer2, 0, RX_BUFFER_SIZE); // 清空缓冲区
            }
        }

        HAL_UART_Receive_IT(&huart2, &rxTemp2, 1); // 再次启动接收中断
    }
}
