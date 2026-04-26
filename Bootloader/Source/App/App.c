#include "App.h"
#include "Bootloader.h"

#include <stdbool.h>

void App_Start()
{
    if (Bootloader_Verify_Slot(Slot_A) == true)
    {
        Bootloader_Jump_To_App(Slot_A);
    }
    else
    {
        Bootloader_Default();
    }
}
