#include "App.h"
#include "Bootloader.h"

#include <stdbool.h>

void App_Start()
{
    if (Bootloader_Verify_Slot() == true)
    {
        Bootloader_Jump_To_App();
    }
    else
    {
        Bootloader_Default();
    }
}
