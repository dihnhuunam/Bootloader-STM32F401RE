/**
 * @file crc32.c
 * @brief Implements CRC-32/IEEE calculation for RAM and Flash data.
 */
#include "crc32.h"

#include "Image_Config.h"
#include "Debug.h"

#include <stdbool.h>
#include <stddef.h>

#define CRC32_INIT_VALUE 0xFFFFFFFFUL
#define CRC32_FINAL_XOR 0xFFFFFFFFUL
#define CRC32_POLY_REVERSED 0xEDB88320UL

static bool Crc32_Is_Flash_Range_Valid(uint32_t flash_addr, uint32_t length)
{
    if ((flash_addr < FLASH_BASE_ADDR) || (flash_addr > FLASH_END_ADDR))
    {
        return false;
    }

    if (length == 0U)
    {
        return true;
    }

    if (flash_addr == FLASH_END_ADDR)
    {
        return false;
    }

    return length <= (FLASH_END_ADDR - flash_addr);
}

uint32_t Crc32_Init(void)
{
    return CRC32_INIT_VALUE;
}

Crc32_Status_t Crc32_Update(uint32_t *crc, const uint8_t *data, uint32_t length)
{
    if (crc == NULL)
    {
        return CRC32_STATUS_INVALID_ARGUMENT;
    }

    if ((data == NULL) && (length != 0U))
    {
        return CRC32_STATUS_INVALID_ARGUMENT;
    }

    while (length-- > 0U)
    {
        *crc ^= *data++;

        for (uint8_t bit = 0U; bit < 8U; bit++)
        {
            if ((*crc & 1U) != 0U)
            {
                *crc = (*crc >> 1U) ^ CRC32_POLY_REVERSED;
            }
            else
            {
                *crc >>= 1U;
            }
        }
    }

    return CRC32_STATUS_OK;
}

Crc32_Status_t Crc32_Finalize(uint32_t *crc)
{
    if (crc == NULL)
    {
        return CRC32_STATUS_INVALID_ARGUMENT;
    }

    *crc ^= CRC32_FINAL_XOR;
    return CRC32_STATUS_OK;
}

Crc32_Status_t Crc32_Calculate(const uint8_t *data, uint32_t length, uint32_t *crc32)
{
    uint32_t crc;
    Crc32_Status_t status;

    if (crc32 == NULL)
    {
        return CRC32_STATUS_INVALID_ARGUMENT;
    }

    if ((data == NULL) && (length != 0U))
    {
        return CRC32_STATUS_INVALID_ARGUMENT;
    }

    crc = Crc32_Init();
    status = Crc32_Update(&crc, data, length);
    if (status != CRC32_STATUS_OK)
    {
        return status;
    }

    status = Crc32_Finalize(&crc);
    if (status != CRC32_STATUS_OK)
    {
        return status;
    }

    *crc32 = crc;
    Debug("CRC32 = 0x%X\n", *crc32);
    return CRC32_STATUS_OK;
}

Crc32_Status_t Crc32_Calculate_From_Flash(uint32_t flash_addr, uint32_t length, uint32_t *crc32)
{
    const uint8_t *flash_data;

    if (crc32 == NULL)
    {
        return CRC32_STATUS_INVALID_ARGUMENT;
    }

    if (!Crc32_Is_Flash_Range_Valid(flash_addr, length))
    {
        return CRC32_STATUS_OUT_OF_RANGE;
    }

    flash_data = (const uint8_t *)(uintptr_t)flash_addr;
    return Crc32_Calculate(flash_data, length, crc32);
}
