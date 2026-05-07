/**
 * @file Image.h
 * @brief Firmware image, vector table, metadata, and OTA request data types.
 */
#ifndef IMAGE_H
#define IMAGE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    /** @brief Firmware image header stored at the start of each application slot. */
    typedef struct __attribute__((packed))
    {
        uint32_t magic;
        uint32_t version;
        uint32_t crc32;
        uint32_t image_size;
    } ImageHeader_t;

    /** @brief Minimal vector table fields used by boot validation and jump code. */
    typedef struct __attribute__((packed))
    {
        uint32_t stack_pointer; // SP address
        uint32_t reset_handler; // Reset Handler address
    } VectorTable_t;

    /** @brief Logical firmware slot identifiers stored in metadata. */
    typedef enum
    {
        IMAGE_SLOT_A = 0U,
        IMAGE_SLOT_B = 1U,
        IMAGE_SLOT_INVALID = 0xFFU
    } ImageSlot_t;

    /** @brief Boot state values used for pending confirmation and rollback. */
    typedef enum
    {
        IMAGE_BOOT_STATE_CONFIRMED = 0U,
        IMAGE_BOOT_STATE_PENDING_CONFIRM = 1U,
        IMAGE_BOOT_STATE_ROLLBACK = 2U
    } ImageBootState_t;

    /** @brief Persistent boot metadata stored in the metadata Flash sector. */
    typedef struct __attribute__((packed, aligned(4)))
    {
        uint32_t magic;
        uint32_t slot_a_crc32;
        uint32_t slot_b_crc32;
        uint32_t firmware_version;
        uint8_t active_slot;
        uint8_t rollback_slot;
        uint8_t pending_slot;
        uint8_t boot_state;
        uint32_t boot_attempts;
    } ImageMetadata_t;

    /** @brief OTA request record written by an app and consumed by the bootloader. */
    typedef struct __attribute__((packed, aligned(4)))
    {
        uint32_t magic;
        uint8_t target_slot;
        uint8_t reserved[3];
        uint32_t image_crc32;
        uint32_t image_size;
    } OtaRequest_t;

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_H */
