/**
 * @file Bootloader.h
 * @brief Application-side boot slot verification and jump helpers.
 */
#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Firmware slot identifiers used by boot helper APIs. */
    typedef enum
    {
        Slot_A = 0,
        Slot_B
    } Bootloader_Slot_t;

    /** @brief Flash layout details for one firmware slot. */
    typedef struct
    {
        uint32_t base_addr;
        uint32_t end_addr;
        uint32_t vector_table_addr;
        uint32_t expected_crc32;
    } Bootloader_Slot_Info_t;

    /**
     * @brief Verifies that a firmware slot contains a bootable image.
     * @param slot Slot to verify.
     * @return true if the slot header, CRC, stack pointer, and reset handler are valid.
     * @return false if verification fails.
     */
    bool Bootloader_Verify_Slot(Bootloader_Slot_t slot);

    /**
     * @brief Transfers execution to an already verified firmware slot.
     * @param slot Slot to jump to.
     */
    void Bootloader_Jump_To_App(Bootloader_Slot_t slot);

    /**
     * @brief Enters the default bootloader fallback loop.
     */
    void Bootloader_Default();

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_H */
