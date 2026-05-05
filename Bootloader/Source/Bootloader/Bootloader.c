#include "Bootloader.h"
#include "Debug.h"
#include "Flash.h"
#include "Image.h"
#include "Image_Config.h"
#include "Led.h"
#include "crc32.h"

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static bool Bootloader_Get_Slot_Info(Bootloader_Slot_t slot, Bootloader_Slot_Info_t *slot_info)
{
    if (slot_info == NULL)
    {
        return false;
    }

    switch (slot)
    {
    case Slot_A:
        slot_info->base_addr = SLOT_A_BASE_ADDR;
        slot_info->end_addr = SLOT_A_END_ADDR;
        slot_info->vector_table_addr = SLOT_A_VECTOR_TABLE_ADDR;
        return true;

    case Slot_B:
        slot_info->base_addr = SLOT_B_BASE_ADDR;
        slot_info->end_addr = SLOT_B_END_ADDR;
        slot_info->vector_table_addr = SLOT_B_VECTOR_TABLE_ADDR;
        return true;

    default:
        return false;
    }
}

static bool Bootloader_Calculate_Slot_Crc32(const ImageHeader_t *header, const Bootloader_Slot_Info_t *slot_info,
                                            uint32_t *calculated_crc32)
{
    Crc32_Status_t crc_status;
    uint32_t max_image_size;

    if ((header == NULL) || (slot_info == NULL) || (calculated_crc32 == NULL))
    {
        return false;
    }

    max_image_size = slot_info->end_addr - slot_info->vector_table_addr;
    if ((header->image_size == 0U) || (header->image_size > max_image_size))
    {
        Debug("Image size invalid: %lu\n", (unsigned long)header->image_size);
        return false;
    }

    crc_status = Crc32_Calculate_From_Flash(slot_info->vector_table_addr, header->image_size, calculated_crc32);
    if (crc_status != CRC32_STATUS_OK)
    {
        Debug("CRC32 calculate failed: status=%d\n", (int)crc_status);
        return false;
    }

    return true;
}

static bool Bootloader_Is_Metadata_Slot_Valid(uint8_t slot)
{
    return (slot == IMAGE_SLOT_A) || (slot == IMAGE_SLOT_B);
}

static Bootloader_Slot_t Bootloader_Metadata_Slot_To_Bootloader_Slot(uint8_t slot)
{
    return (slot == IMAGE_SLOT_B) ? Slot_B : Slot_A;
}

static uint8_t Bootloader_Slot_To_Metadata_Slot(Bootloader_Slot_t slot)
{
    return (slot == Slot_B) ? IMAGE_SLOT_B : IMAGE_SLOT_A;
}

static Bootloader_Slot_t Bootloader_Get_Other_Slot(Bootloader_Slot_t slot)
{
    return (slot == Slot_B) ? Slot_A : Slot_B;
}

static bool Bootloader_Is_Metadata_Valid(const ImageMetadata_t *metadata)
{
    return (metadata != NULL) && (metadata->magic == METADATA_MAGIC_NUMBER) &&
           Bootloader_Is_Metadata_Slot_Valid(metadata->active_slot);
}

static void Bootloader_Read_Metadata(ImageMetadata_t *metadata)
{
    if (metadata != NULL)
    {
        memcpy(metadata, (const void *)METADATA_BASE_ADDR, sizeof(*metadata));
    }
}

static bool Bootloader_Write_Metadata(const ImageMetadata_t *metadata)
{
    if (metadata == NULL)
    {
        return false;
    }

    if (Flash_Erase(METADATA_BASE_ADDR, sizeof(*metadata)) != FLASH_STATUS_OK)
    {
        Debug("Metadata erase failed: 0x%08lX\n", (unsigned long)Flash_GetLastError());
        return false;
    }

    if (Flash_Write(METADATA_BASE_ADDR, (const uint8_t *)metadata, sizeof(*metadata)) != FLASH_STATUS_OK)
    {
        Debug("Metadata write failed: 0x%08lX\n", (unsigned long)Flash_GetLastError());
        return false;
    }

    return true;
}

static bool Bootloader_Select_Verified_Slot(Bootloader_Slot_t candidate, Bootloader_Slot_t *slot)
{
    if ((slot != NULL) && Bootloader_Verify_Slot(candidate))
    {
        *slot = candidate;
        return true;
    }

    return false;
}

static void Bootloader_Init_Metadata_For_Slot(Bootloader_Slot_t slot)
{
    Bootloader_Slot_Info_t slot_info;
    const ImageHeader_t *header;
    ImageMetadata_t metadata = {
        .magic = METADATA_MAGIC_NUMBER,
        .slot_a_crc32 = 0xFFFFFFFFUL,
        .slot_b_crc32 = 0xFFFFFFFFUL,
        .firmware_version = 0xFFFFFFFFUL,
        .active_slot = Bootloader_Slot_To_Metadata_Slot(slot),
        .rollback_slot = IMAGE_SLOT_INVALID,
        .pending_slot = IMAGE_SLOT_INVALID,
        .boot_state = IMAGE_BOOT_STATE_CONFIRMED,
        .boot_attempts = 0U,
    };

    if (!Bootloader_Get_Slot_Info(slot, &slot_info))
    {
        return;
    }

    header = (const ImageHeader_t *)slot_info.base_addr;
    metadata.firmware_version = header->version;

    if (slot == Slot_A)
    {
        metadata.slot_a_crc32 = header->crc32;
    }
    else
    {
        metadata.slot_b_crc32 = header->crc32;
    }

    if (Bootloader_Write_Metadata(&metadata))
    {
        Debug("Metadata initialized for Slot %c\n", (slot == Slot_A) ? 'A' : 'B');
    }
}

static bool Bootloader_Rollback_To_Slot(ImageMetadata_t *metadata, Bootloader_Slot_t rollback_slot,
                                        Bootloader_Slot_t *slot)
{
    if (!Bootloader_Select_Verified_Slot(rollback_slot, slot))
    {
        return false;
    }

    if (metadata != NULL)
    {
        metadata->active_slot = Bootloader_Slot_To_Metadata_Slot(rollback_slot);
        metadata->pending_slot = IMAGE_SLOT_INVALID;
        metadata->rollback_slot = IMAGE_SLOT_INVALID;
        metadata->boot_state = IMAGE_BOOT_STATE_CONFIRMED;
        metadata->boot_attempts = 0U;
        (void)Bootloader_Write_Metadata(metadata);
    }

    Debug("Rollback slot selected\n");
    return true;
}

static bool Bootloader_Is_Stack_Pointer_Valid(uint32_t stack_pointer)
{
    return (stack_pointer >= SRAM_START_ADDR) && (stack_pointer <= SRAM_END_ADDR) && ((stack_pointer & 0x7U) == 0U);
}

static bool Bootloader_Is_Reset_Handler_Valid(uint32_t reset_handler, const Bootloader_Slot_Info_t *slot_info)
{
    return (slot_info != NULL) && (reset_handler >= slot_info->vector_table_addr) &&
           (reset_handler < slot_info->end_addr) && ((reset_handler & 0x1U) != 0U);
}

bool Bootloader_Verify_Slot(Bootloader_Slot_t slot)
{
    Bootloader_Slot_Info_t slot_info;
    uint32_t calculated_crc32 = 0U;

    if (!Bootloader_Get_Slot_Info(slot, &slot_info))
    {
        Debug("Invalid boot slot\n");
        return false;
    }

    const ImageHeader_t *header = (const ImageHeader_t *)slot_info.base_addr;

    if (header->magic != APP_MAGIC_NUMBER)
    {
        Debug("Magic number incorrect!\n");
        return false;
    }

    if (!Bootloader_Calculate_Slot_Crc32(header, &slot_info, &calculated_crc32))
    {
        return false;
    }

    if (header->crc32 != calculated_crc32)
    {
        Debug("CRC32 incorrect: header=0x%08lX calc=0x%08lX\n", (unsigned long)header->crc32,
              (unsigned long)calculated_crc32);
        return false;
    }

    // Verify Stack Pointer
    const VectorTable_t *vtable = (const VectorTable_t *)slot_info.vector_table_addr;
    if (!Bootloader_Is_Stack_Pointer_Valid(vtable->stack_pointer))
    {
        Debug("Stack pointer is invalid\n");
        return false;
    }

    if (!Bootloader_Is_Reset_Handler_Valid(vtable->reset_handler, &slot_info))
    {
        Debug("Reset handler is invalid\n");
        return false;
    }

    // Application Verified Successfully
    Debug("Application verified\n");
    return true;
}

bool Bootloader_Select_Boot_Slot(Bootloader_Slot_t *slot)
{
    ImageMetadata_t metadata;
    Bootloader_Slot_t active_slot;
    Bootloader_Slot_t pending_slot;
    Bootloader_Slot_t rollback_slot;
    bool has_pending;

    if (slot == NULL)
    {
        return false;
    }

    Bootloader_Read_Metadata(&metadata);
    if (!Bootloader_Is_Metadata_Valid(&metadata))
    {
        Debug("Metadata invalid, fallback to available slot\n");
        if (Bootloader_Select_Verified_Slot(Slot_A, slot))
        {
            Bootloader_Init_Metadata_For_Slot(Slot_A);
            return true;
        }

        if (Bootloader_Select_Verified_Slot(Slot_B, slot))
        {
            Bootloader_Init_Metadata_For_Slot(Slot_B);
            return true;
        }

        return false;
    }

    active_slot = Bootloader_Metadata_Slot_To_Bootloader_Slot(metadata.active_slot);
    has_pending = (metadata.boot_state == IMAGE_BOOT_STATE_PENDING_CONFIRM) &&
                  Bootloader_Is_Metadata_Slot_Valid(metadata.pending_slot);

    if (has_pending)
    {
        pending_slot = Bootloader_Metadata_Slot_To_Bootloader_Slot(metadata.pending_slot);

        if (metadata.boot_attempts >= METADATA_MAX_BOOT_ATTEMPTS)
        {
            Debug("Pending image exceeded boot attempts\n");
            rollback_slot = Bootloader_Is_Metadata_Slot_Valid(metadata.rollback_slot)
                                ? Bootloader_Metadata_Slot_To_Bootloader_Slot(metadata.rollback_slot)
                                : active_slot;
            return Bootloader_Rollback_To_Slot(&metadata, rollback_slot, slot);
        }

        if (Bootloader_Verify_Slot(pending_slot))
        {
            metadata.boot_attempts++;
            (void)Bootloader_Write_Metadata(&metadata);
            *slot = pending_slot;
            return true;
        }

        rollback_slot = Bootloader_Is_Metadata_Slot_Valid(metadata.rollback_slot)
                            ? Bootloader_Metadata_Slot_To_Bootloader_Slot(metadata.rollback_slot)
                            : active_slot;
        return Bootloader_Rollback_To_Slot(&metadata, rollback_slot, slot);
    }

    if ((metadata.boot_state != IMAGE_BOOT_STATE_ROLLBACK) && Bootloader_Select_Verified_Slot(active_slot, slot))
    {
        return true;
    }

    if (Bootloader_Is_Metadata_Slot_Valid(metadata.rollback_slot))
    {
        rollback_slot = Bootloader_Metadata_Slot_To_Bootloader_Slot(metadata.rollback_slot);
        if (rollback_slot != active_slot)
        {
            return Bootloader_Rollback_To_Slot(&metadata, rollback_slot, slot);
        }
    }

    rollback_slot = Bootloader_Get_Other_Slot(active_slot);
    if (Bootloader_Select_Verified_Slot(rollback_slot, slot))
    {
        metadata.active_slot = Bootloader_Slot_To_Metadata_Slot(rollback_slot);
        metadata.pending_slot = IMAGE_SLOT_INVALID;
        metadata.rollback_slot = IMAGE_SLOT_INVALID;
        metadata.boot_state = IMAGE_BOOT_STATE_CONFIRMED;
        metadata.boot_attempts = 0U;
        (void)Bootloader_Write_Metadata(&metadata);
        return true;
    }

    return false; /* Safe mode/recovery will be implemented later. */
}

void Bootloader_Jump_To_App(Bootloader_Slot_t slot)
{
    Bootloader_Slot_Info_t slot_info;

    if (!Bootloader_Get_Slot_Info(slot, &slot_info))
    {
        Debug("Invalid boot slot\n");
        return;
    }

    // Get Stack Pointer * Reset Handler address from Application Vector Table
    const VectorTable_t *vtable = (const VectorTable_t *)slot_info.vector_table_addr;
    uint32_t app_stack_pointer_addr = vtable->stack_pointer;
    uint32_t app_reset_handler_addr = vtable->reset_handler;

    // Function pointer to Reset_Handler
    void (*app_reset_handler)(void);
    app_reset_handler = (void *)app_reset_handler_addr;

    // Disable all interrupts
    __disable_irq();

    // Deregister SysTick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    // Remove pending interrupts
    for (int i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF; /* Disable */
        NVIC->ICPR[i] = 0xFFFFFFFF; /* Clear pending */
    }

    // Set Application's Vector Table
    SCB->VTOR = slot_info.vector_table_addr;
    __DSB();
    __ISB();

    // Set Stack Pointer
    __set_MSP(app_stack_pointer_addr);

    // Let the application initialize its own tick and peripheral interrupts.
    __enable_irq();

    // Start Application's Reset Handler
    app_reset_handler();
}

void Bootloader_Default()
{
    Debug("Bootloader default\n");
    while (1)
    {
        Led_Blink(200);
    }
}
