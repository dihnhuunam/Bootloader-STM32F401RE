#include "Bootloader.h"
#include "Debug.h"
#include "Image.h"
#include "Led.h"

#include "stm32f4xx_hal.h"
#include <stdbool.h>

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
    const VectorTable_t *vtable = (const VectorTable_t *)SLOT_A_BASE_ADDR;
    if ((vtable->stack_pointer < 0x20000000UL) || (vtable->stack_pointer > 0x20018000UL))
    {
        Debug("Stack pointer is invalid\n");
        return false;
    }

    // Application Verified Successfully
    Debug("Application verifed\n");
    return true;
}

void Bootloader_Jump_To_App()
{
    // Get Stack Pointer * Reset Handler address from Application Vector Table
    uint32_t app_stack_pointer_addr = *((volatile uint32_t *)(SLOT_A_BASE_ADDR));
    uint32_t app_reset_handler_addr = *((volatile uint32_t *)(SLOT_A_BASE_ADDR + 4UL));

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
    SCB->VTOR = SLOT_A_BASE_ADDR;
    __DSB();
    __ISB();

    // Set Stack Pointer
    __set_MSP(app_stack_pointer_addr);

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
