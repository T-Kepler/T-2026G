/* 串口HMI板级示例：UART和HMI页面需先在CubeMX/屏端配置完成。 */
#include "my_hmi.h"

void example_hmi(UART_HandleTypeDef hmi_uart)
{
    uint8_t wave_data[16];

    HMI_Send_String(hmi_uart, "p0.t0", "采集就绪");
    HMI_Send_Int(hmi_uart, "p0.n0", 10);
    HMI_Send_Float(hmi_uart, "p0.x0", 10.5f, 1);

    for (int i = 0; i < 16; ++i)
    {
        wave_data[i] = i * 16; /* HMI波形数据范围为0～255。 */
        HMI_Write_Wave_Low(hmi_uart, "s0", 0, wave_data[i]);
    }

    HMI_Wave_Clear(hmi_uart, "s0", 0);
    HMI_Write_Wave_Fast(hmi_uart, "s0", 0, 16, wave_data);
}
