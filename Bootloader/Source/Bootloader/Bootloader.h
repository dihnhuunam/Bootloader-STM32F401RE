/**
 * @file Bootloader.h
 * @brief Bootloader slot selection, verification, boot mode, and jump APIs.
 */
#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Identifies an application firmware slot.
     */
    typedef enum
    {
        Slot_A = 0, /**< Application Slot A. */
        Slot_B      /**< Application Slot B. */
    } Bootloader_Slot_t;

    /**
     * @brief Describes the fixed Flash layout of an application slot.
     */
    typedef struct
    {
        uint32_t base_addr;         /**< Slot base address containing the image header. */
        uint32_t end_addr;          /**< Exclusive end address of the slot. */
        uint32_t vector_table_addr; /**< Application vector table address. */
    } Bootloader_Slot_Info_t;

    /**
     * @brief Verifies whether a firmware slot contains a bootable image.
     * @param slot Slot to verify.
     * @return true if image header, CRC, stack pointer, and reset handler are valid.
     * @return false if the slot is invalid or verification fails.
     */
    bool Bootloader_Verify_Slot(Bootloader_Slot_t slot);

    /**
     * @brief Selects the firmware slot to boot according to metadata state.
     * @param slot Output selected slot.
     * @return true if a bootable slot was selected.
     * @return false if no valid slot is available.
     */
    bool Bootloader_Select_Boot_Slot(Bootloader_Slot_t *slot);

    /**
     * @brief Checks whether Boot Mode should be entered.
     * @return true if the user button is held for the required duration.
     * @return false otherwise.
     */
    bool Bootloader_Should_Enter_Boot_Mode(void);

    /**
     * @brief Runs the manual Boot Mode CLI.
     *
     * The CLI lets the user select Slot A or Slot B manually. If the selected
     * slot verifies successfully, execution is transferred to that slot.
     */
    void Bootloader_Run_Boot_Mode(void);

    /**
     * @brief Transfers execution to the selected application slot.
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
