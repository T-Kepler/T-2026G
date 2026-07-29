#include "key_app.h"

#include "g_signal_app.h"

#define NUM_KEYS 4U
#define KEY_DEBOUNCE_MS 30U

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    GSignalControl control;
} KeyDefinition;

static const KeyDefinition keys[NUM_KEYS] = {
    {KEY1_GPIO_Port, KEY1_Pin, G_SIGNAL_CONTROL_MODE_NEXT},
    {KEY2_GPIO_Port, KEY2_Pin, G_SIGNAL_CONTROL_PERIOD_TOGGLE},
    {KEY3_GPIO_Port, KEY3_Pin, G_SIGNAL_CONTROL_VIEW_TOGGLE},
    {KEY4_GPIO_Port, KEY4_Pin, G_SIGNAL_CONTROL_MEASURE}
};

static uint8_t readKeyMap(void)
{
    uint8_t key_map = 0U;
    for (uint8_t index = 0U; index < NUM_KEYS; index++)
    {
        if (HAL_GPIO_ReadPin(keys[index].port, keys[index].pin) ==
            GPIO_PIN_RESET)
            key_map |= 1U << index;
    }
    return key_map;
}

void key_proc(void)
{
    static uint8_t candidate_map;
    static uint8_t stable_map;
    static uint32_t candidate_since;
    uint8_t current_map = readKeyMap();
    uint32_t now = HAL_GetTick();

    if (current_map != candidate_map)
    {
        candidate_map = current_map;
        candidate_since = now;
        return;
    }
    if (candidate_map == stable_map ||
        now - candidate_since < KEY_DEBOUNCE_MS)
        return;

    uint8_t pressed = candidate_map & (uint8_t)~stable_map;
    stable_map = candidate_map;
    for (uint8_t index = 0U; index < NUM_KEYS; index++)
    {
        if ((pressed & (1U << index)) != 0U)
            GSignalApp_HandleControl(keys[index].control);
    }
}
