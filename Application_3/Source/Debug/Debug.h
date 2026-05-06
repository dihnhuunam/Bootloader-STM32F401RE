/**
 * @file Debug.h
 * @brief UART debug output helpers.
 */
#ifndef DEBUG_H
#define DEBUG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    /**
     * @brief Prints a formatted debug message through the configured UART.
     * @param format printf-style format string.
     * @param ... Format arguments.
     */
    void Debug(const char *format, ...);

    /**
     * @brief Prints a byte buffer as hexadecimal values with an optional prefix.
     * @param ptr Input byte buffer.
     * @param len Number of bytes to print.
     * @param ... Optional printf-style prefix format followed by its arguments.
     */
    void Debug_Hex(char *ptr, uint8_t len, ...);

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_H */
