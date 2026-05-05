#include "App.h"
#include "Bootloader.h"

#include <stdbool.h>

void App_Start()
{
    Bootloader_Slot_t slot;

    if (Bootloader_Should_Enter_Boot_Mode())
    {
        Bootloader_Run_Boot_Mode();
    }

    if (Bootloader_Select_Boot_Slot(&slot) == true)
    {
        Bootloader_Jump_To_App(slot);
    }
    else
    {
        Bootloader_Default();
    }
}
