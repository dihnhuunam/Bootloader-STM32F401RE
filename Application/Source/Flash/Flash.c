/**
 * @file Flash.c
 * @brief Implements bounded Flash read, erase, write, and verification helpers.
 */
#include "Flash.h"

#include "Image_Config.h"
#include "stm32f4xx_hal.h"

#include <string.h>

/** @brief Marker used when an address does not map to a supported STM32F401 sector. */
#define FLASH_INVALID_SECTOR 0xFFFFFFFFUL

/** @brief Last HAL error code, failed erase sector, or verify-failed address. */
static uint32_t s_flash_last_error = 0U;

/**
 * @brief Checks whether a range is fully inside a half-open interval.
 * @param address Start address of the range.
 * @param length Number of bytes in the range. A zero length is accepted.
 * @param start Inclusive start address of the valid interval.
 * @param end Exclusive end address of the valid interval.
 * @return true if `[address, address + length)` is inside `[start, end)`.
 * @return false if the range starts outside the interval or overflows the interval end.
 */
static bool Flash_IsRangeInside(uint32_t address, uint32_t length, uint32_t start, uint32_t end)
{
    if ((address < start) || (address > end))
    {
        return false;
    }

    if (length == 0U)
    {
        return true;
    }

    if (address == end)
    {
        return false;
    }

    return length <= (end - address);
}

/**
 * @brief Checks whether a range is fully inside MCU Flash.
 * @param address Start address of the range to check.
 * @param length Number of bytes in the range.
 * @return true if the range is inside MCU Flash, false otherwise.
 */
bool Flash_IsAddressRangeInFlash(uint32_t address, uint32_t length)
{
    return Flash_IsRangeInside(address, length, FLASH_BASE_ADDR, FLASH_END_ADDR);
}

/**
 * @brief Checks whether a range is inside metadata, Slot A, or Slot B.
 * @param address Start address of the range to check.
 * @param length Number of bytes in the range.
 * @return true if the range can be modified by this module, false otherwise.
 */
bool Flash_IsAddressRangeWritable(uint32_t address, uint32_t length)
{
    return Flash_IsRangeInside(address, length, METADATA_BASE_ADDR, METADATA_END_ADDR) ||
           Flash_IsRangeInside(address, length, SLOT_A_BASE_ADDR, SLOT_A_END_ADDR) ||
           Flash_IsRangeInside(address, length, SLOT_B_BASE_ADDR, SLOT_B_END_ADDR);
}

/**
 * @brief Converts an STM32F401 Flash address to a HAL sector number.
 * @param address Flash address to map.
 * @return FLASH_SECTOR_0 through FLASH_SECTOR_7 for valid 512 KB Flash addresses.
 * @return FLASH_INVALID_SECTOR if address is outside supported Flash.
 */
static uint32_t Flash_GetSector(uint32_t address)
{
    if ((address >= 0x08000000UL) && (address < 0x08004000UL))
    {
        return FLASH_SECTOR_0;
    }
    if (address < 0x08008000UL)
    {
        return FLASH_SECTOR_1;
    }
    if (address < 0x0800C000UL)
    {
        return FLASH_SECTOR_2;
    }
    if (address < 0x08010000UL)
    {
        return FLASH_SECTOR_3;
    }
    if (address < 0x08020000UL)
    {
        return FLASH_SECTOR_4;
    }
    if (address < 0x08040000UL)
    {
        return FLASH_SECTOR_5;
    }
    if (address < 0x08060000UL)
    {
        return FLASH_SECTOR_6;
    }
    if (address < FLASH_END_ADDR)
    {
        return FLASH_SECTOR_7;
    }

    return FLASH_INVALID_SECTOR;
}

/**
 * @brief Clears sticky Flash operation and error flags before a new operation.
 */
static void Flash_ClearErrorFlags(void)
{
    uint32_t flags = FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR |
                     FLASH_FLAG_PGSERR;

#ifdef FLASH_FLAG_RDERR
    flags |= FLASH_FLAG_RDERR;
#endif

    __HAL_FLASH_CLEAR_FLAG(flags);
}

/**
 * @brief Copies bytes from Flash memory to RAM.
 * @param address Flash source address.
 * @param buffer Destination RAM buffer.
 * @param length Number of bytes to copy.
 * @return FLASH_STATUS_OK, FLASH_STATUS_INVALID_ARGUMENT, or FLASH_STATUS_OUT_OF_RANGE.
 */
Flash_Status_t Flash_Read(uint32_t address, uint8_t *buffer, uint32_t length)
{
    if (length == 0U)
    {
        return FLASH_STATUS_OK;
    }

    if (buffer == NULL)
    {
        return FLASH_STATUS_INVALID_ARGUMENT;
    }

    if (!Flash_IsAddressRangeInFlash(address, length))
    {
        return FLASH_STATUS_OUT_OF_RANGE;
    }

    memcpy(buffer, (const void *)(uintptr_t)address, length);
    return FLASH_STATUS_OK;
}

/**
 * @brief Erases all Flash sectors intersecting a writable range.
 * @param address Start address of the target erase range.
 * @param length Number of bytes in the target erase range.
 * @return FLASH_STATUS_OK, FLASH_STATUS_OUT_OF_RANGE, or FLASH_STATUS_HAL_ERROR.
 */
Flash_Status_t Flash_Erase(uint32_t address, uint32_t length)
{
    FLASH_EraseInitTypeDef erase_init = {0};
    uint32_t sector_error = 0U;
    uint32_t first_sector;
    uint32_t last_sector;
    Flash_Status_t status = FLASH_STATUS_OK;

    if (length == 0U)
    {
        return FLASH_STATUS_OK;
    }

    if (!Flash_IsAddressRangeWritable(address, length))
    {
        return FLASH_STATUS_OUT_OF_RANGE;
    }

    first_sector = Flash_GetSector(address);
    last_sector = Flash_GetSector(address + length - 1U);

    if ((first_sector == FLASH_INVALID_SECTOR) || (last_sector == FLASH_INVALID_SECTOR))
    {
        return FLASH_STATUS_OUT_OF_RANGE;
    }

    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector = first_sector;
    erase_init.NbSectors = (last_sector - first_sector) + 1U;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        s_flash_last_error = HAL_FLASH_GetError();
        return FLASH_STATUS_HAL_ERROR;
    }

    Flash_ClearErrorFlags();

    if (HAL_FLASHEx_Erase(&erase_init, &sector_error) != HAL_OK)
    {
        s_flash_last_error = sector_error;
        status = FLASH_STATUS_HAL_ERROR;
    }

    if ((HAL_FLASH_Lock() != HAL_OK) && (status == FLASH_STATUS_OK))
    {
        s_flash_last_error = HAL_FLASH_GetError();
        status = FLASH_STATUS_HAL_ERROR;
    }

    return status;
}

/**
 * @brief Programs bytes to Flash and verifies each programmed byte.
 * @param address Flash destination address.
 * @param data Source buffer.
 * @param length Number of bytes to program.
 * @return FLASH_STATUS_OK, FLASH_STATUS_INVALID_ARGUMENT, FLASH_STATUS_OUT_OF_RANGE,
 * FLASH_STATUS_HAL_ERROR, or FLASH_STATUS_VERIFY_FAILED.
 */
Flash_Status_t Flash_Write(uint32_t address, const uint8_t *data, uint32_t length)
{
    Flash_Status_t status = FLASH_STATUS_OK;

    if (length == 0U)
    {
        return FLASH_STATUS_OK;
    }

    if (data == NULL)
    {
        return FLASH_STATUS_INVALID_ARGUMENT;
    }

    if (!Flash_IsAddressRangeWritable(address, length))
    {
        return FLASH_STATUS_OUT_OF_RANGE;
    }

    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        s_flash_last_error = HAL_FLASH_GetError();
        return FLASH_STATUS_HAL_ERROR;
    }

    Flash_ClearErrorFlags();

    for (uint32_t index = 0U; index < length; index++)
    {
        uint32_t target_address = address + index;

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, target_address, data[index]) != HAL_OK)
        {
            s_flash_last_error = HAL_FLASH_GetError();
            status = FLASH_STATUS_HAL_ERROR;
            break;
        }

        if (*(const volatile uint8_t *)(uintptr_t)target_address != data[index])
        {
            s_flash_last_error = target_address;
            status = FLASH_STATUS_VERIFY_FAILED;
            break;
        }
    }

    if ((HAL_FLASH_Lock() != HAL_OK) && (status == FLASH_STATUS_OK))
    {
        s_flash_last_error = HAL_FLASH_GetError();
        status = FLASH_STATUS_HAL_ERROR;
    }

    return status;
}

/**
 * @brief Returns the latest low-level error detail captured by this module.
 * @return HAL error code, sector error value, or address that failed verification.
 */
uint32_t Flash_GetLastError(void)
{
    return s_flash_last_error;
}
