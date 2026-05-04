#ifndef UART_FIRMWARE_UPDATE_H
#define UART_FIRMWARE_UPDATE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "Bootloader.h"
#include "stm32f4xx_hal.h"

    typedef enum
    {
        UART_FW_UPDATE_STATUS_OK = 0,
        UART_FW_UPDATE_STATUS_NO_UPDATE,
        UART_FW_UPDATE_STATUS_INVALID_ARGUMENT,
        UART_FW_UPDATE_STATUS_INVALID_HEADER,
        UART_FW_UPDATE_STATUS_IMAGE_TOO_LARGE,
        UART_FW_UPDATE_STATUS_UART_ERROR,
        UART_FW_UPDATE_STATUS_FLASH_ERROR
    } UartFirmwareUpdate_Status_t;

    UartFirmwareUpdate_Status_t UartFirmwareUpdate_Receive_To_Slot(UART_HandleTypeDef *huart,
                                                                   Bootloader_Slot_t target_slot);

#ifdef __cplusplus
}
#endif

#endif /* UART_FIRMWARE_UPDATE_H */
