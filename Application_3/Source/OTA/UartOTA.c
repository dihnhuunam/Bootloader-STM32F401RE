/**
 * @file UartOTA.c
 * @brief UART OTA receiver using START/ACK/NACK block flow control.
 */
#include "UartOTA.h"

#include "Debug.h"
#include "Flash.h"
#include "Image.h"
#include "Image_Config.h"
#include "usart.h"

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define UART_OTA_START_BYTE 0x7EU
#define UART_OTA_ACK 0x79U
#define UART_OTA_NACK 0x1FU
#define UART_OTA_PACKET_MAGIC 0x3341544FUL
#define UART_OTA_BLOCK_SIZE 256U
#define UART_OTA_RX_TIMEOUT_MS 5000U

typedef struct
{
    uint8_t slot;
    uint32_t base_addr;
    uint32_t size;
} UartOTA_SlotInfo_t;

static uint8_t s_uartota_buffer[UART_OTA_BLOCK_SIZE];
static uint8_t s_uartota_start_byte;
static volatile bool s_uartota_start_requested = false;
static volatile bool s_uartota_start_rx_active = false;

/**
 * @brief Sends a one-byte OTA flow-control response.
 * @param response Response byte, usually UART_OTA_ACK or UART_OTA_NACK.
 * @return true if the byte was transmitted.
 * @return false if UART transmit failed.
 */
static bool UartOTA_Send_Response(uint8_t response)
{
    return HAL_UART_Transmit(&huart1, &response, 1U, UART_OTA_RX_TIMEOUT_MS) == HAL_OK;
}

/**
 * @brief Arms interrupt reception for one OTA START byte.
 */
static void UartOTA_Start_Receive_IT(void)
{
    if (!s_uartota_start_rx_active)
    {
        s_uartota_start_rx_active = true;
        if (HAL_UART_Receive_IT(&huart1, &s_uartota_start_byte, 1U) != HAL_OK)
        {
            s_uartota_start_rx_active = false;
        }
    }
}

/**
 * @brief Selects the slot that is not currently running.
 * @param active_slot Current image slot value.
 * @param slot_info Output inactive slot layout.
 * @return true when active_slot is valid and slot_info was filled.
 * @return false on invalid arguments or unknown slot value.
 */
static bool UartOTA_Get_Inactive_Slot(uint8_t active_slot, UartOTA_SlotInfo_t *slot_info)
{
    if (slot_info == NULL)
    {
        return false;
    }

    if (active_slot == IMAGE_SLOT_A)
    {
        slot_info->slot = IMAGE_SLOT_B;
        slot_info->base_addr = SLOT_B_BASE_ADDR;
        slot_info->size = SLOT_B_SIZE;
        return true;
    }

    if (active_slot == IMAGE_SLOT_B)
    {
        slot_info->slot = IMAGE_SLOT_A;
        slot_info->base_addr = SLOT_A_BASE_ADDR;
        slot_info->size = SLOT_A_SIZE;
        return true;
    }

    return false;
}

/**
 * @brief Receives an exact number of bytes from the OTA UART.
 * @param data Destination buffer.
 * @param length Number of bytes to receive.
 * @return true if all bytes were received before timeout.
 * @return false on UART timeout or error.
 */
static bool UartOTA_Read_Exact(uint8_t *data, uint32_t length)
{
    while (length > 0U)
    {
        uint16_t chunk = (length > UART_OTA_BLOCK_SIZE) ? UART_OTA_BLOCK_SIZE : (uint16_t)length;

        if (HAL_UART_Receive(&huart1, data, chunk, UART_OTA_RX_TIMEOUT_MS) != HAL_OK)
        {
            return false;
        }

        data += chunk;
        length -= chunk;
    }

    return true;
}

/**
 * @brief Receives OTA image data, writes it to Flash, and validates its header.
 * @param slot_info Target inactive slot selected by the running firmware.
 * @param image_size Number of bytes to receive and program.
 * @param image_header Output header copied from the received image.
 * @return true if the image was received, written, and validated.
 * @return false on UART, Flash, or image validation failure.
 */
static bool UartOTA_Receive_And_Write_Image(const UartOTA_SlotInfo_t *slot_info, uint32_t image_size,
                                            ImageHeader_t *image_header)
{
    uint32_t offset = 0U;

    if ((slot_info == NULL) || (image_header == NULL) || (image_size < sizeof(ImageHeader_t)) ||
        (image_size > slot_info->size))
    {
        return false;
    }

    memset(image_header, 0, sizeof(*image_header));

    if (Flash_Erase(slot_info->base_addr, slot_info->size) != FLASH_STATUS_OK)
    {
        Debug("OTA erase failed: 0x%08lX\n", (unsigned long)Flash_GetLastError());
        return false;
    }

    (void)UartOTA_Send_Response(UART_OTA_ACK);

    while (offset < image_size)
    {
        uint32_t remaining = image_size - offset;
        uint32_t chunk = (remaining > UART_OTA_BLOCK_SIZE) ? UART_OTA_BLOCK_SIZE : remaining;

        if (!UartOTA_Read_Exact(s_uartota_buffer, chunk))
        {
            Debug("OTA receive failed\n");
            (void)UartOTA_Send_Response(UART_OTA_NACK);
            return false;
        }

        if (offset < sizeof(ImageHeader_t))
        {
            uint32_t header_remaining = sizeof(ImageHeader_t) - offset;
            uint32_t header_copy = (chunk < header_remaining) ? chunk : header_remaining;
            memcpy(((uint8_t *)image_header) + offset, s_uartota_buffer, header_copy);
        }

        if (Flash_Write(slot_info->base_addr + offset, s_uartota_buffer, chunk) != FLASH_STATUS_OK)
        {
            Debug("OTA write failed: 0x%08lX\n", (unsigned long)Flash_GetLastError());
            (void)UartOTA_Send_Response(UART_OTA_NACK);
            return false;
        }

        (void)UartOTA_Send_Response(UART_OTA_ACK);
        offset += chunk;
    }

    if ((image_header->magic != APP_MAGIC_NUMBER) || (image_header->image_size == 0U) ||
        (image_header->image_size > (slot_info->size - VTABLE_OFFSET)))
    {
        Debug("OTA image header invalid\n");
        (void)UartOTA_Send_Response(UART_OTA_NACK);
        return false;
    }

    return true;
}

/**
 * @brief Writes an OTA request record for the bootloader to process on next reset.
 * @param slot_info Slot containing the newly written image.
 * @param image_header Header of the newly written image.
 * @return true if the request was erased and written successfully.
 * @return false if Flash erase or write failed.
 */
static bool UartOTA_Write_Request(const UartOTA_SlotInfo_t *slot_info, const ImageHeader_t *image_header)
{
    OtaRequest_t request = {
        .magic = OTA_REQUEST_MAGIC,
        .target_slot = slot_info->slot,
        .reserved = {0xFFU, 0xFFU, 0xFFU},
        .image_crc32 = image_header->crc32,
        .image_size = image_header->image_size,
    };

    if (Flash_Erase(OTA_REQUEST_BASE_ADDR, OTA_REQUEST_SIZE) != FLASH_STATUS_OK)
    {
        Debug("OTA request erase failed: 0x%08lX\n", (unsigned long)Flash_GetLastError());
        return false;
    }

    if (Flash_Write(OTA_REQUEST_BASE_ADDR, (const uint8_t *)&request, sizeof(request)) != FLASH_STATUS_OK)
    {
        Debug("OTA request write failed: 0x%08lX\n", (unsigned long)Flash_GetLastError());
        return false;
    }

    return true;
}

/**
 * @brief Runs one MCU-driven OTA transaction.
 * @param active_slot Current running image slot.
 * @return true if the inactive slot was updated and an OTA request was written.
 * @return false on protocol, UART, Flash, or image validation failure.
 */
static bool UartOTA_Handle_Update(uint8_t active_slot)
{
    UartOTA_SlotInfo_t target_slot;
    ImageHeader_t image_header;
    uint32_t image_size;

    if (!UartOTA_Get_Inactive_Slot(active_slot, &target_slot))
    {
        Debug("OTA active slot invalid\n");
        (void)UartOTA_Send_Response(UART_OTA_NACK);
        return false;
    }

    (void)UartOTA_Send_Response(UART_OTA_ACK);
    if (!UartOTA_Send_Response(target_slot.slot))
    {
        return false;
    }

    if (!UartOTA_Read_Exact((uint8_t *)&image_size, sizeof(image_size)))
    {
        Debug("OTA image size receive failed\n");
        (void)UartOTA_Send_Response(UART_OTA_NACK);
        return false;
    }

    if ((image_size < sizeof(ImageHeader_t)) || (image_size > target_slot.size))
    {
        Debug("OTA image size invalid: %lu\n", (unsigned long)image_size);
        (void)UartOTA_Send_Response(UART_OTA_NACK);
        return false;
    }

    (void)UartOTA_Send_Response(UART_OTA_ACK);

    if (!UartOTA_Receive_And_Write_Image(&target_slot, image_size, &image_header))
    {
        return false;
    }

    if (!UartOTA_Write_Request(&target_slot, &image_header))
    {
        (void)UartOTA_Send_Response(UART_OTA_NACK);
        return false;
    }

    (void)UartOTA_Send_Response(UART_OTA_ACK);
    Debug("OTA image stored for Slot %c, pending next reset\n", (target_slot.slot == IMAGE_SLOT_A) ? 'A' : 'B');
    return true;
}

/**
 * @brief Arms the OTA receiver for a future START byte.
 */
void UartOTA_Init(void)
{
    s_uartota_start_requested = false;
    s_uartota_start_rx_active = false;
    UartOTA_Start_Receive_IT();
}

/**
 * @brief Processes a pending START byte and performs OTA if requested.
 * @param active_slot Current running image slot.
 */
void UartOTA_Process(uint8_t active_slot)
{
    if (!s_uartota_start_requested)
    {
        UartOTA_Start_Receive_IT();
        return;
    }

    __disable_irq();
    s_uartota_start_requested = false;
    __enable_irq();

    Debug("UART OTA start\n");
    if (!UartOTA_Handle_Update(active_slot))
    {
        Debug("UART OTA failed\n");
    }

    UartOTA_Start_Receive_IT();
}

/**
 * @brief HAL callback invoked when the START byte interrupt receive completes.
 * @param huart UART handle that completed reception.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1)
    {
        return;
    }

    s_uartota_start_rx_active = false;
    if (s_uartota_start_byte == UART_OTA_START_BYTE)
    {
        s_uartota_start_requested = true;
        return;
    }

    UartOTA_Start_Receive_IT();
}

/**
 * @brief HAL callback invoked when UART reception reports an error.
 * @param huart UART handle that reported an error.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        s_uartota_start_rx_active = false;
        UartOTA_Start_Receive_IT();
    }
}
