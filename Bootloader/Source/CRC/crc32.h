/**
 * @file crc32.h
 * @brief CRC-32/IEEE calculation helpers for bootloader image validation.
 * @details
 * This module implements the reflected CRC-32/IEEE algorithm used by
 * Python zlib.crc32(). It can calculate CRC over RAM buffers or directly
 * over memory-mapped internal Flash.
 */
#ifndef CRC32_H
#define CRC32_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    /**
     * @brief Result code returned by CRC32 module operations.
     */
    typedef enum
    {
        CRC32_STATUS_OK = 0,           /**< CRC calculation completed successfully. */
        CRC32_STATUS_INVALID_ARGUMENT, /**< A required pointer or argument is invalid. */
        CRC32_STATUS_OUT_OF_RANGE      /**< Requested Flash range is outside MCU Flash. */
    } Crc32_Status_t;

    /**
     * @brief Initializes a streaming CRC32 calculation.
     * @return Initial CRC32 accumulator value.
     */
    uint32_t Crc32_Init(void);

    /**
     * @brief Updates a streaming CRC32 accumulator with a byte buffer.
     * @param crc CRC32 accumulator to update in place.
     * @param data Input byte buffer.
     * @param length Number of bytes to process.
     * @return CRC32_STATUS_OK on success.
     * @return CRC32_STATUS_INVALID_ARGUMENT if crc is NULL, or data is NULL while length is non-zero.
     * @note If length is zero, data may be NULL and crc is returned unchanged.
     */
    Crc32_Status_t Crc32_Update(uint32_t *crc, const uint8_t *data, uint32_t length);

    /**
     * @brief Finalizes a streaming CRC32 calculation.
     * @param crc CRC32 accumulator to finalize in place.
     * @return CRC32_STATUS_OK on success.
     * @return CRC32_STATUS_INVALID_ARGUMENT if crc is NULL.
     * @note After finalization, *crc contains the final value compatible with zlib.crc32().
     */
    Crc32_Status_t Crc32_Finalize(uint32_t *crc);

    /**
     * @brief Calculates CRC32 over a RAM buffer.
     * @param data Input byte buffer.
     * @param length Number of bytes to process.
     * @param crc32 Output CRC32 value.
     * @return CRC32_STATUS_OK on success.
     * @return CRC32_STATUS_INVALID_ARGUMENT if crc32 is NULL, or data is NULL while length is non-zero.
     */
    Crc32_Status_t Crc32_Calculate(const uint8_t *data, uint32_t length, uint32_t *crc32);

    /**
     * @brief Calculates CRC32 directly from memory-mapped internal Flash.
     * @param flash_addr Start address in internal Flash.
     * @param length Number of bytes to process.
     * @param crc32 Output CRC32 value.
     * @return CRC32_STATUS_OK on success.
     * @return CRC32_STATUS_INVALID_ARGUMENT if crc32 is NULL.
     * @return CRC32_STATUS_OUT_OF_RANGE if the Flash range is outside internal Flash.
     * @note The result matches Python zlib.crc32() over the same byte sequence.
     */
    Crc32_Status_t Crc32_Calculate_From_Flash(uint32_t flash_addr, uint32_t length, uint32_t *crc32);

#ifdef __cplusplus
}
#endif

#endif /* CRC32_H */
