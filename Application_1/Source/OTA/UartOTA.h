#ifndef UART_OTA_H
#define UART_OTA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    void UartOTA_Init(void);
    void UartOTA_Process(uint8_t active_slot);

#ifdef __cplusplus
}
#endif

#endif /* UART_OTA_H */
