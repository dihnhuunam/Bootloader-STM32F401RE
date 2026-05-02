#include "Bootloader.h"
#include "Debug.h"
#include "Image.h"
#include "Image_Config.h"
#include "Led.h"
#include "crc32.h"

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stddef.h>

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
        slot_info->calculated_crc32 = 0U;
        return true;

    case Slot_B:
        slot_info->base_addr = SLOT_B_BASE_ADDR;
        slot_info->end_addr = SLOT_B_END_ADDR;
        slot_info->vector_table_addr = SLOT_B_VECTOR_TABLE_ADDR;
        slot_info->calculated_crc32 = 0U;
        return true;

    default:
        return false;
    }
}

static bool Bootloader_Calculate_Slot_Crc32(const ImageHeader_t *header, Bootloader_Slot_Info_t *slot_info)
{
    Crc32_Status_t crc_status;
    uint32_t max_image_size;

    if ((header == NULL) || (slot_info == NULL))
    {
        return false;
    }

    max_image_size = slot_info->end_addr - slot_info->vector_table_addr;
    if ((header->image_size == 0U) || (header->image_size > max_image_size))
    {
        Debug("Image size invalid: %lu\n", (unsigned long)header->image_size);
        return false;
    }

    crc_status =
        Crc32_Calculate_From_Flash(slot_info->vector_table_addr, header->image_size, &slot_info->calculated_crc32);
    if (crc_status != CRC32_STATUS_OK)
    {
        Debug("CRC32 calculate failed: status=%d\n", (int)crc_status);
        return false;
    }

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
#ifdef DEMO_ROLLBACK
    if (slot == Slot_B)
    {
        Debug("[DEMO] Simulating Slot B failure (CRC mismatch).\n");
        return false;
    }
#endif

    Bootloader_Slot_Info_t slot_info;

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

    if (!Bootloader_Calculate_Slot_Crc32(header, &slot_info))
    {
        return false;
    }

    if (header->crc32 != slot_info.calculated_crc32)
    {
        Debug("CRC32 incorrect: header=0x%08lX calc=0x%08lX\n", (unsigned long)header->crc32,
              (unsigned long)slot_info.calculated_crc32);
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

void Bootloader_Jump_To_App(Bootloader_Slot_t slot)
{
    Debug("Jump to Application\n");
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
