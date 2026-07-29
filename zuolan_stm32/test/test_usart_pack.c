/* 运行时模板数据帧板级示例。 */
#include "my_usart_pack.h"

void example_usart_pack(UART_HandleTypeDef data_uart)
{
    uint8_t state = 1U;
    uint16_t count = 25U;
    float voltage = 3.3f;
    DataType types[] = {TYPE_BYTE, TYPE_SHORT, TYPE_FLOAT};
    void *send_values[] = {&state, &count, &voltage};
    uint8_t frame[32];
    uint16_t frame_length;

    SetParseTemplate(types, send_values, 3U);
    frame_length = PrepareFrame(frame, sizeof(frame));
    if (frame_length == 0U)
    {
        return;
    }
    SendFrame(data_uart, frame, frame_length);

    /* 回环演示：实际项目中frame来自串口接收缓冲区。 */
    state = 0U;
    count = 0U;
    voltage = 0.0f;
    ParseFrame(frame, frame_length);
}
