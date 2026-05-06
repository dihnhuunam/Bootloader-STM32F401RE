/**
 * @file Led.h
 * @brief LED control helpers for status indication.
 */
#ifndef LED_H
#define LED_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    /** @brief Initializes the LED helper state. */
    void Led_Init(void);

    /** @brief Turns the status LED on. */
    void Led_On(void);

    /** @brief Turns the status LED off. */
    void Led_Off(void);

    /** @brief Toggles the status LED state. */
    void Led_Toggle(void);

    /**
     * @brief Blinks the LED using a polling-based period check.
     * @param period_ms Blink period in milliseconds.
     */
    void Led_Blink(uint32_t period_ms);

#ifdef __cplusplus
}
#endif

#endif /* LED_H */
