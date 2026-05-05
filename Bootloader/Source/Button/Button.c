/**
 * @file Button.c
 * @brief Implements helper APIs for reading the boot selection button.
 */
#include "Button.h"

#include "main.h"

#define BUTTON_PORT GPIOC
#define BUTTON_PIN GPIO_PIN_13
#define BUTTON_PRESSED_STATE GPIO_PIN_RESET
#define BUTTON_DEBOUNCE_TIME_MS 20U

/**
 * @brief Initializes the button helper state.
 *
 * @note PC13 is already configured as GPIO input with pull-up by MX_GPIO_Init().
 */
void Button_Init(void)
{
}

/**
 * @brief Checks whether the user button is currently pressed.
 *
 * @return true if PC13 reads the active pressed level.
 * @return false if PC13 reads the released level.
 */
bool Button_Is_Pressed(void)
{
    return HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == BUTTON_PRESSED_STATE;
}

/**
 * @brief Checks whether the user button remains pressed for a minimum time.
 *
 * @param hold_time_ms Required hold time in milliseconds.
 *
 * @return true if the button stayed pressed for the full hold time.
 * @return false if the button was released before the hold time elapsed.
 */
bool Button_Is_Held(uint32_t hold_time_ms)
{
    uint32_t start_tick;

    if (!Button_Is_Pressed())
    {
        return false;
    }

    HAL_Delay(BUTTON_DEBOUNCE_TIME_MS);
    if (!Button_Is_Pressed())
    {
        return false;
    }

    start_tick = HAL_GetTick();
    while ((HAL_GetTick() - start_tick) < hold_time_ms)
    {
        if (!Button_Is_Pressed())
        {
            return false;
        }
    }

    return true;
}
