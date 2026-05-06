/**
 * @file UartOTA.h
 * @brief UART OTA update receiver interface.
 */
#ifndef UART_OTA_H
#define UART_OTA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    /**
     * @brief Arms UART interrupt reception for the OTA START byte.
     */
    void UartOTA_Init(void);

    /**
     * @brief Processes a pending OTA session for the running firmware slot.
     * @param active_slot Currently running image slot, using ImageSlot_t values.
     */
    void UartOTA_Process(uint8_t active_slot);

#ifdef __cplusplus
}
#endif

#endif /* UART_OTA_H */
