#include "App.h"
#include "Debug.h"
#include "Image.h"
#include "Led.h"
#include "stm32f4xx_hal.h"

#include <stdbool.h>

const ImageHeader_t __app_header __attribute__((section(".app_header"), used)) = {
    .magic = APP_MAGIC_NUMBER, .version = (0x00010000), .crc32 = 0x00000000, .reserved = 0};

void App_Start()
{
    while (1)
    {
        Led_Blink(3000);
        Debug("Application Firmware 1\n");
        HAL_Delay(3000);
    }
}
