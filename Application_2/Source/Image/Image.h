#ifndef IMAGE_H
#define IMAGE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    typedef struct __attribute__((packed))
    {
        uint32_t magic;
        uint32_t version;
        uint32_t crc32;
        uint32_t image_size;
    } ImageHeader_t;

    typedef struct __attribute__((packed))
    {
        uint32_t stack_pointer; // SP address
        uint32_t reset_handler; // Reset Handler address
    } VectorTable_t;

    typedef enum
    {
        IMAGE_SLOT_A = 0U,
        IMAGE_SLOT_B = 1U,
        IMAGE_SLOT_INVALID = 0xFFU
    } ImageSlot_t;

    typedef enum
    {
        IMAGE_BOOT_STATE_CONFIRMED = 0U,
        IMAGE_BOOT_STATE_PENDING_CONFIRM = 1U,
        IMAGE_BOOT_STATE_ROLLBACK = 2U
    } ImageBootState_t;

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

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_H */
