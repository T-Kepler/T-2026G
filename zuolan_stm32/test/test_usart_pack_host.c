#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "my_usart_pack.h"


static uint8_t transmitted[32];
static uint16_t transmitted_length;

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *data,
                                    uint16_t length, uint32_t timeout)
{
    (void)huart;
    (void)timeout;
    memcpy(transmitted, data, length);
    transmitted_length = length;
    return HAL_OK;
}

int main(void)
{
    uint8_t state = 0x12U;
    uint16_t count = 0x3456U;
    int value = 0x12345678;
    float voltage = 3.25f;
    DataType types[] = {TYPE_BYTE, TYPE_SHORT, TYPE_INT, TYPE_FLOAT};
    void *variables[] = {&state, &count, &value, &voltage};
    uint8_t frame[32];

    SetParseTemplate(types, variables, 4U);
    uint16_t length = PrepareFrame(frame, sizeof(frame));
    assert(length == 14U);
    assert(frame[0] == FRAME_HEADER && frame[length - 1] == FRAME_TAIL);
    assert(frame[1] == 0x12U);
    assert(frame[2] == 0x34U && frame[3] == 0x56U);
    assert(frame[4] == 0x12U && frame[5] == 0x34U);
    assert(frame[6] == 0x56U && frame[7] == 0x78U);
    assert(PrepareFrame(frame, length - 1U) == 0U);

    state = 0U;
    count = 0U;
    value = 0;
    voltage = 0.0f;
    ParseFrame(frame, length);
    assert(state == 0x12U && count == 0x3456U && value == 0x12345678);
    assert(fabsf(voltage - 3.25f) < 1e-6f);

    state = 0U;
    frame[length - 2] ^= 1U;
    ParseFrame(frame, length);
    assert(state == 0U);
    frame[length - 2] ^= 1U;

    UART_HandleTypeDef uart = {0};
    SendFrame(uart, frame, length);
    assert(transmitted_length == length);
    assert(memcmp(transmitted, frame, length) == 0);

    puts("USART frame protocol test passed");
    return 0;
}
