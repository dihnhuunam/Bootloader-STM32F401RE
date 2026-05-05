/**
 * @file Button.h
 * @brief Button helper APIs for reading the boot selection button.
 */
#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

    /**
     * @brief Initializes the button helper state.
     *
     * @note The GPIO pin itself is configured by MX_GPIO_Init().
     */
    void Button_Init(void);

    /**
     * @brief Checks whether the user button is currently pressed.
     *
     * @return true if the button is pressed.
     * @return false if the button is released.
     */
    bool Button_Is_Pressed(void);

    /**
     * @brief Checks whether the user button remains pressed for a minimum time.
     *
     * @param hold_time_ms Required hold time in milliseconds.
     *
     * @return true if the button stayed pressed for the full hold time.
     * @return false if the button was released before the hold time elapsed.
     */
    bool Button_Is_Held(uint32_t hold_time_ms);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H */
