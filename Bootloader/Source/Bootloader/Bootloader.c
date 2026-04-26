#include "Bootloader.h"
#include "Debug.h"
#include "Image.h"
#include "Image_Config.h"
#include "Led.h"

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    uint32_t base_addr;
    uint32_t end_addr;
    uint32_t vector_table_addr;
    uint32_t expected_crc32;
} Bootloader_Slot_Info_t;

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
        slot_info->expected_crc32 = SLOT_A_CRC_32;
        return true;

    case Slot_B:
        slot_info->base_addr = SLOT_B_BASE_ADDR;
        slot_info->end_addr = SLOT_B_END_ADDR;
        slot_info->vector_table_addr = SLOT_B_VECTOR_TABLE_ADDR;
        slot_info->expected_crc32 = SLOT_B_CRC_32;
        return true;

    default:
        return false;
    }
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

    // Verify Application Firmware's CRC32

    if (header->crc32 != slot_info.expected_crc32)
    {
        Debug("CRC32 incorrect!\n");
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
