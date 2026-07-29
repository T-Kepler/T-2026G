/*******************************************************************************
 * @file      my_usart_pack.h
 * @author    左岚
 * @version   V1.0
 * @date      2025-07-18
 * @brief     基于运行时模板的数据帧打包与解析接口
 * @note      整数按大端序，float沿用MCU原生字节序。
 *******************************************************************************/

#ifndef __MY_USART_PACK_H__
#define __MY_USART_PACK_H__

#include "my_usart.h" // 包含自定义的USART底层驱动头文件，以获取UART_HandleTypeDef等定义。
#include "stdint.h"   // 包含标准整型类型定义，如 uint8_t, uint16_t 等。

// --------------------------- 宏定义 ---------------------------

// 定义帧头和帧尾，用于数据包的同步和校验。
#define FRAME_HEADER 0xA5 // 帧起始标志
#define FRAME_TAIL   0x5A // 帧结束标志
#define MIN_FRAME_LENGTH 3 // 定义一个有效数据帧的最小长度（帧头 + 校验和 + 帧尾）

// 定义支持的数据类型枚举
typedef enum
{
    TYPE_BYTE,  // 1字节数据类型 (uint8_t / int8_t)
    TYPE_SHORT, // 2字节数据类型 (uint16_t / int16_t)
    TYPE_INT,   // 4字节数据类型 (uint32_t / int32_t)
    TYPE_FLOAT  // 4字节浮点数类型 (float)
} DataType;

// 定义解析模板中变量的最大数量，防止数组越界。
#define MAX_VARIABLES 10

// --------------------------- 外部函数声明 ---------------------------

/**
 * @brief     设置数据帧的解析/打包模板。
 * @details   此函数用于定义通信数据帧的结构。通过两个数组，一个指定每个变量的
 * 数据类型，另一个指定存储这些变量的内存地址，来建立一个映射关系。
 * 后续的打包和解包函数都将依据此模板进行操作。
 * @param     templateArray 指向一个 `DataType` 枚举数组的指针，定义了数据帧中各个变量的类型。
 * @param     variableArray 指向一个 `void*` 数组的指针，每个元素指向一个实际变量的地址。
 * @param     count         模板中变量的数量，不能超过 `MAX_VARIABLES`。
 */
void SetParseTemplate(DataType *templateArray, void **variableArray, uint16_t count);

/**
 * @brief     解析接收到的原始数据帧。
 * @details   该函数接收一个原始的字节缓冲区，根据预先通过 `SetParseTemplate` 设置的模板，
 * 从中提取有效数据，并将其存入模板指定的变量地址中。函数会自动处理
 * 帧头、帧尾和校验和的验证。
 * @param     buffer        指向包含原始串口数据的缓冲区的指针。
 * @param     length        缓冲区中数据的长度。
 */
void ParseFrame(uint8_t *buffer, uint16_t length);

/**
 * @brief     根据模板准备（打包）一帧待发送的数据。
 * @details   该函数会根据预设的模板，从模板指定的变量地址中读取数据，然后按照
 * 协议格式（帧头、数据、校验和、帧尾）将它们打包到一个发送缓冲区中。
 * @param     buffer        用于存放打包后数据帧的缓冲区指针。
 * @param     maxLength     缓冲区的最大长度，以防止溢出。
 * @return    uint16_t      返回打包完成后，实际数据帧的长度。如果长度为0，表示打包失败。
 */
uint16_t PrepareFrame(uint8_t *buffer, uint16_t maxLength);

/**
 * @brief     通过指定的UART端口发送一个数据帧。
 * @param     huart_blue    用于发送数据的UART句柄 (例如 &huart1)。
 * @param     frame         指向已经打包好的数据帧缓冲区的指针。
 * @param     length        要发送的数据帧的长度。
 */
void SendFrame(UART_HandleTypeDef huart_blue, uint8_t *frame, uint16_t length);

#endif // __MY_USART_PACK_H__
