/**
 * @file Debug.c
 * @brief Implements UART-based debug output helpers.
 */
#include "Debug.h"
#include "usart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define CLI_DEBUG_BUFFER_SIZE 256U

/**
 * @brief Sends a raw debug message through UART.
 * @param message Message buffer.
 * @param length Message length
 * in bytes.
 */
static void Cli_Debug_Send(const char *message, uint16_t length)
{
    if ((message == NULL) || (length == 0U))
    {
        return;
    }

    HAL_UART_Transmit(&huart2, (uint8_t *)message, length, HAL_MAX_DELAY);
}

/**
 * @brief Prints a formatted debug message through UART.
 * @param format printf-style format string.
 * @param ...
 * Format arguments.
 */
void Debug(const char *format, ...)
{
    va_list args;
    int written;
    char buffer[CLI_DEBUG_BUFFER_SIZE];

    if (format == NULL)
    {
        return;
    }

    va_start(args, format);
    written = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (written <= 0)
    {
        return;
    }

    if ((size_t)written >= sizeof(buffer))
    {
        written = (int)(sizeof(buffer) - 1U);
    }

    Cli_Debug_Send(buffer, (uint16_t)written);
}

/**
 * @brief Prints a byte buffer as hexadecimal values with an optional prefix.
 * @param ptr Input byte buffer.
 *
 * @param len Number of bytes to print.
 * @param ... Optional printf-style prefix format followed by its arguments.
 */
void Debug_Hex(char *ptr, uint8_t len, ...)
{
    va_list args;
    const char *prefix_format;
    char prefix[CLI_DEBUG_BUFFER_SIZE];
    char hex_buffer[4];
    int written;
    uint8_t index;

    if ((ptr == NULL) || (len == 0U))
    {
        return;
    }

    va_start(args, len);
    prefix_format = va_arg(args, const char *);
    if (prefix_format != NULL)
    {
        written = vsnprintf(prefix, sizeof(prefix), prefix_format, args);
        if (written > 0)
        {
            if ((size_t)written >= sizeof(prefix))
            {
                written = (int)(sizeof(prefix) - 1U);
            }
            Cli_Debug_Send(prefix, (uint16_t)written);
        }
    }
    va_end(args);

    for (index = 0U; index < len; index++)
    {
        written = snprintf(hex_buffer, sizeof(hex_buffer), "%02X ", (uint8_t)ptr[index]);
        if (written > 0)
        {
            Cli_Debug_Send(hex_buffer, (uint16_t)written);
        }
    }

    Cli_Debug_Send("\r\n", 2U);
}
