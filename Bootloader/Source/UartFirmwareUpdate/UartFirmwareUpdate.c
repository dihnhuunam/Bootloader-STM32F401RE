#include "UartFirmwareUpdate.h"

#include "Debug.h"
#include "Flash.h"
#include "Image_Config.h"

#include <stdbool.h>
#include <stdint.h>

#define UART_FW_UPDATE_START_BYTE 0xAAU
#define UART_FW_UPDATE_ACK 0x79U
#define UART_FW_UPDATE_NACK 0x1FU
#define UART_FW_UPDATE_HEADER_SIZE 3U
#define UART_FW_UPDATE_CHUNK_SIZE 256U
#define UART_FW_UPDATE_START_TIMEOUT_MS 3000U
#define UART_FW_UPDATE_BYTE_TIMEOUT_MS 1000U

typedef struct
{
    uint32_t base_addr;
    uint32_t size;
    const char *name;
} UartFirmwareUpdate_Target_t;

static bool UartFirmwareUpdate_GetTarget(Bootloader_Slot_t slot, UartFirmwareUpdate_Target_t *target)
{
    if (target == NULL)
    {
        return false;
    }

    switch (slot)
    {
    case Slot_A:
        target->base_addr = SLOT_A_BASE_ADDR;
        target->size = SLOT_A_SIZE;
        target->name = "Slot A";
        return true;

    case Slot_B:
        target->base_addr = SLOT_B_BASE_ADDR;
        target->size = SLOT_B_SIZE;
        target->name = "Slot B";
        return true;

    default:
        return false;
    }
}

static uint16_t UartFirmwareUpdate_Checksum(const uint8_t *data, uint16_t length)
{
    uint16_t checksum = 0U;

    for (uint16_t index = 0U; index < length; index++)
    {
        checksum = (uint16_t)(checksum + data[index]);
    }

    return checksum;
}

static void UartFirmwareUpdate_SendByte(UART_HandleTypeDef *huart, uint8_t value)
{
    (void)HAL_UART_Transmit(huart, &value, 1U, HAL_MAX_DELAY);
}

static UartFirmwareUpdate_Status_t UartFirmwareUpdate_ReceivePacket(UART_HandleTypeDef *huart, uint8_t *data,
                                                                    uint16_t *length, uint32_t start_timeout)
{
    uint8_t header[UART_FW_UPDATE_HEADER_SIZE];
    uint8_t crc_buf[2];
    uint16_t checksum_recv;
    uint16_t checksum_calc;

    if (HAL_UART_Receive(huart, &header[0], 1U, start_timeout) != HAL_OK)
    {
        return UART_FW_UPDATE_STATUS_NO_UPDATE;
    }

    if (header[0] != UART_FW_UPDATE_START_BYTE)
    {
        return UART_FW_UPDATE_STATUS_INVALID_HEADER;
    }

    if (HAL_UART_Receive(huart, &header[1], 2U, UART_FW_UPDATE_BYTE_TIMEOUT_MS) != HAL_OK)
    {
        return UART_FW_UPDATE_STATUS_UART_ERROR;
    }

    *length = ((uint16_t)header[1] << 8U) | (uint16_t)header[2];
    if (*length > UART_FW_UPDATE_CHUNK_SIZE)
    {
        return UART_FW_UPDATE_STATUS_IMAGE_TOO_LARGE;
    }

    if (*length == 0U)
    {
        return UART_FW_UPDATE_STATUS_OK;
    }

    if (HAL_UART_Receive(huart, data, *length, UART_FW_UPDATE_BYTE_TIMEOUT_MS) != HAL_OK)
    {
        return UART_FW_UPDATE_STATUS_UART_ERROR;
    }

    if (HAL_UART_Receive(huart, crc_buf, 2U, UART_FW_UPDATE_BYTE_TIMEOUT_MS) != HAL_OK)
    {
        return UART_FW_UPDATE_STATUS_UART_ERROR;
    }

    checksum_recv = ((uint16_t)crc_buf[0] << 8U) | (uint16_t)crc_buf[1];
    checksum_calc = UartFirmwareUpdate_Checksum(data, *length);
    if (checksum_recv != checksum_calc)
    {
        return UART_FW_UPDATE_STATUS_INVALID_HEADER;
    }

    return UART_FW_UPDATE_STATUS_OK;
}

UartFirmwareUpdate_Status_t UartFirmwareUpdate_Receive_To_Slot(UART_HandleTypeDef *huart, Bootloader_Slot_t target_slot)
{
    uint8_t chunk[UART_FW_UPDATE_CHUNK_SIZE];
    uint16_t chunk_size;
    uint32_t bytes_written = 0U;
    Flash_Status_t flash_status;
    UartFirmwareUpdate_Status_t packet_status;
    UartFirmwareUpdate_Target_t target;

    if ((huart == NULL) || !UartFirmwareUpdate_GetTarget(target_slot, &target))
    {
        return UART_FW_UPDATE_STATUS_INVALID_ARGUMENT;
    }

    packet_status = UartFirmwareUpdate_ReceivePacket(huart, chunk, &chunk_size, UART_FW_UPDATE_START_TIMEOUT_MS);
    if (packet_status != UART_FW_UPDATE_STATUS_OK)
    {
        UartFirmwareUpdate_SendByte(huart, UART_FW_UPDATE_NACK);
        return packet_status;
    }

    if (chunk_size == 0U)
    {
        UartFirmwareUpdate_SendByte(huart, UART_FW_UPDATE_ACK);
        return UART_FW_UPDATE_STATUS_NO_UPDATE;
    }

    flash_status = Flash_Erase(target.base_addr, target.size);
    if (flash_status != FLASH_STATUS_OK)
    {
        Debug("%s erase failed: %d\n", target.name, (int)flash_status);
        UartFirmwareUpdate_SendByte(huart, UART_FW_UPDATE_NACK);
        return UART_FW_UPDATE_STATUS_FLASH_ERROR;
    }

    while (1)
    {
        if ((bytes_written + chunk_size) > target.size)
        {
            UartFirmwareUpdate_SendByte(huart, UART_FW_UPDATE_NACK);
            return UART_FW_UPDATE_STATUS_IMAGE_TOO_LARGE;
        }

        flash_status = Flash_Write(target.base_addr + bytes_written, chunk, chunk_size);
        if (flash_status != FLASH_STATUS_OK)
        {
            Debug("%s write failed: %d\n", target.name, (int)flash_status);
            UartFirmwareUpdate_SendByte(huart, UART_FW_UPDATE_NACK);
            return UART_FW_UPDATE_STATUS_FLASH_ERROR;
        }

        bytes_written += chunk_size;
        UartFirmwareUpdate_SendByte(huart, UART_FW_UPDATE_ACK);

        packet_status = UartFirmwareUpdate_ReceivePacket(huart, chunk, &chunk_size, HAL_MAX_DELAY);
        if (packet_status != UART_FW_UPDATE_STATUS_OK)
        {
            UartFirmwareUpdate_SendByte(huart, UART_FW_UPDATE_NACK);
            return packet_status;
        }

        if (chunk_size == 0U)
        {
            UartFirmwareUpdate_SendByte(huart, UART_FW_UPDATE_ACK);
            Debug("UART update %s size=%lu\n", target.name, (unsigned long)bytes_written);
            return UART_FW_UPDATE_STATUS_OK;
        }
    }
}
