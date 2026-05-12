/**
 * @file Led.c
 * @brief Implements status LED helper functions.
 */
#include "Led.h"

#include "main.h"

#define LED_PORT GPIOA
#define LED_PIN GPIO_PIN_5

/** @brief Stores the last tick used by the polling-based blink helper. */
static uint32_t s_led_tick = 0U;

/**
 * @brief Initializes the LED helper state.
 */
void Led_Init(void)
{
    Led_Off();
    s_led_tick = HAL_GetTick();
}

/**
 * @brief Turns the status LED on.
 */
void Led_On(void)
{
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/**
 * @brief Turns the status LED off.
 */
void Led_Off(void)
{
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

/**
 * @brief Toggles the status LED state.
 */
void Led_Toggle(void)
{
    HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
}

/**
 * @brief Blinks the LED using a polling-based period check.
 * @param period_ms Blink period in milliseconds.
 */
void Led_Blink(uint32_t period_ms)
{
    uint32_t now = HAL_GetTick();

    if ((now - s_led_tick) >= period_ms)
    {
        s_led_tick = now;
        Led_Toggle();
    }
}
