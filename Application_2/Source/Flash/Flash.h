/**
 * @file Flash.h
 * @brief Flash memory access interface for the bootloader.
 * @details
 * Provides bounded Flash read, sector erase, byte programming, and write
 * verification helpers. Write and erase operations are limited to bootloader
 * writable regions: metadata, Slot A, and Slot B.
 */
#ifndef FLASH_H
#define FLASH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

    /**
     * @brief Result code returned by Flash module operations.
     */
    typedef enum
    {
        FLASH_STATUS_OK = 0,           /**< Operation completed successfully. */
        FLASH_STATUS_INVALID_ARGUMENT, /**< A required pointer or argument is invalid. */
        FLASH_STATUS_OUT_OF_RANGE,     /**< Address range is outside the allowed region. */
        FLASH_STATUS_HAL_ERROR,        /**< STM32 HAL Flash driver reported an error. */
        FLASH_STATUS_VERIFY_FAILED     /**< Programmed byte did not match read-back value. */
    } Flash_Status_t;

    /**
     * @brief Checks whether a range is fully inside MCU Flash.
     * @param address Start address of the range to check.
     * @param length Number of bytes in the range. A zero length is accepted.
     * @return true if the complete range is inside `[FLASH_BASE_ADDR, FLASH_END_ADDR)`.
     * @return false if any byte of the range is outside MCU Flash.
     */
    bool Flash_IsAddressRangeInFlash(uint32_t address, uint32_t length);

    /**
     * @brief Checks whether a range can be modified by bootloader Flash APIs.
     * @param address Start address of the range to check.
     * @param length Number of bytes in the range. A zero length is accepted.
     * @return true if the complete range is inside metadata, Slot A, or Slot B.
     * @return false if the range touches bootloader Flash or any unsupported area.
     */
    bool Flash_IsAddressRangeWritable(uint32_t address, uint32_t length);

    /**
     * @brief Copies bytes from Flash memory to a RAM buffer.
     * @param address Flash source address.
     * @param buffer Destination RAM buffer.
     * @param length Number of bytes to read. Zero returns FLASH_STATUS_OK.
     * @return FLASH_STATUS_OK if the read completed.
     * @return FLASH_STATUS_INVALID_ARGUMENT if buffer is NULL and length is non-zero.
     * @return FLASH_STATUS_OUT_OF_RANGE if the requested range is outside MCU Flash.
     */
    Flash_Status_t Flash_Read(uint32_t address, uint8_t *buffer, uint32_t length);

    /**
     * @brief Erases every Flash sector touched by a writable address range.
     * @param address Start address of the range to erase.
     * @param length Number of bytes in the target range. Zero returns FLASH_STATUS_OK.
     * @return FLASH_STATUS_OK if all touched sectors were erased.
     * @return FLASH_STATUS_OUT_OF_RANGE if the range is not bootloader-writable.
     * @return FLASH_STATUS_HAL_ERROR if unlock, erase, or lock fails.
     * @note Erase granularity is sector-based; bytes outside the requested range but inside
     * the same touched sectors will also be erased by the STM32 Flash controller.
     */
    Flash_Status_t Flash_Erase(uint32_t address, uint32_t length);

    /**
     * @brief Programs bytes to Flash and verifies each byte by read-back.
     * @param address Flash destination address.
     * @param data Source buffer containing bytes to program.
     * @param length Number of bytes to program. Zero returns FLASH_STATUS_OK.
     * @return FLASH_STATUS_OK if all bytes were programmed and verified.
     * @return FLASH_STATUS_INVALID_ARGUMENT if data is NULL and length is non-zero.
     * @return FLASH_STATUS_OUT_OF_RANGE if the destination range is not bootloader-writable.
     * @return FLASH_STATUS_HAL_ERROR if unlock, program, or lock fails.
     * @return FLASH_STATUS_VERIFY_FAILED if a programmed byte fails read-back verification.
     * @note The destination range must already be erased before programming.
     */
    Flash_Status_t Flash_Write(uint32_t address, const uint8_t *data, uint32_t length);

    /**
     * @brief Gets the latest low-level Flash error detail.
     * @return HAL Flash error code, failed erase sector, or verify-failed address.
     * @note The meaning depends on the last failed operation.
     */
    uint32_t Flash_GetLastError(void);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_H */
