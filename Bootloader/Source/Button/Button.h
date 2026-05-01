/**
 * @file Button.h
 * @brief User button (PC13) polling helpers.
 *
 * The user button on the STM32F401RE Nucleo board is wired to PC13 with an
 * external pull-up resistor; the pin reads LOW when the button is pressed.
 * GPIO must be initialised by MX_GPIO_Init() before calling any function here.
 */
#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

    /** @brief Resets the internal hold-timer state. Call once before the polling loop. */
    void Button_Init(void);

    /** @brief Returns true if the user button is currently pressed. */
    bool Button_IsPressed(void);

    /**
     * @brief Returns true if the button has been held continuously for at least duration_ms.
     *
     * The timer starts when the button transitions from released to pressed and resets
     * whenever the button is released.  Must be called repeatedly in a polling loop.
     *
     * @param duration_ms Minimum hold duration in milliseconds.
     */
    bool Button_IsHeldFor(uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H */
