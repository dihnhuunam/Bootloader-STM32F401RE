/**
 * @file Button.c
 * @brief Implements user button (PC13) polling helpers.
 */
#include "Button.h"

#include "stm32f4xx_hal.h"

#define BUTTON_PORT GPIOC
#define BUTTON_PIN  GPIO_PIN_13

static uint32_t s_press_start_tick = 0U;
static bool     s_is_counting      = false;

void Button_Init(void)
{
    s_press_start_tick = 0U;
    s_is_counting      = false;
}

bool Button_IsPressed(void)
{
    /* Active-low: pin reads RESET when the button is pressed. */
    return HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET;
}

bool Button_IsHeldFor(uint32_t duration_ms)
{
    bool     pressed = Button_IsPressed();
    uint32_t now     = HAL_GetTick();

    if (pressed)
    {
        if (!s_is_counting)
        {
            s_press_start_tick = now;
            s_is_counting      = true;
        }
    }
    else
    {
        s_is_counting = false;
    }

    return s_is_counting && ((now - s_press_start_tick) >= duration_ms);
}
