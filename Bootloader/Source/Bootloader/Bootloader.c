#include "Bootloader.h"
#include "Debug.h"
#include "Image.h"
#include "Led.h"

#include "stm32f4xx_hal.h"
#include <stdbool.h>

static bool Bootloader_Is_Stack_Pointer_Valid(uint32_t stack_pointer)
{
    return (stack_pointer >= SRAM_START_ADDR) && (stack_pointer <= SRAM_END_ADDR) && ((stack_pointer & 0x7U) == 0U);
}

static bool Bootloader_Is_Reset_Handler_Valid(uint32_t reset_handler)
{
    return (reset_handler >= APP_VECTOR_TABLE_ADDR) && (reset_handler < SLOT_A_END_ADDR) &&
           ((reset_handler & 0x1U) != 0U);
}

bool Bootloader_Verify_Slot()
{
    // Verify Magic Number
    const ImageHeader_t *header = (ImageHeader_t *)SLOT_A_BASE_ADDR;

    if (header->magic != APP_MAGIC_NUMBER)
    {
        Debug("Magic number incorrect!\n");
        return false;
    }

    // Verify Stack Pointer
    const VectorTable_t *vtable = (const VectorTable_t *)APP_VECTOR_TABLE_ADDR;
    if (!Bootloader_Is_Stack_Pointer_Valid(vtable->stack_pointer))
    {
        Debug("Stack pointer is invalid\n");
        return false;
    }

    if (!Bootloader_Is_Reset_Handler_Valid(vtable->reset_handler))
    {
        Debug("Reset handler is invalid\n");
        return false;
    }

    // Application Verified Successfully
    Debug("Application verified\n");
    return true;
}

void Bootloader_Jump_To_App()
{
    // Get Stack Pointer * Reset Handler address from Application Vector Table
    const VectorTable_t *vtable = (const VectorTable_t *)APP_VECTOR_TABLE_ADDR;
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
    SCB->VTOR = APP_VECTOR_TABLE_ADDR;
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
