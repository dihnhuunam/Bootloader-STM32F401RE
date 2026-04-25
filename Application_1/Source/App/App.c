#include "App.h"
#include "Debug.h"
#include "Image.h"
#include "Led.h"

#include <stdbool.h>

APP_HEADER_DEFINE(0x00010000);

void App_Start()
{
    while (1)
    {
        Led_Blink(3000);
        Debug("Application Firmware 1\n");
    }
}
