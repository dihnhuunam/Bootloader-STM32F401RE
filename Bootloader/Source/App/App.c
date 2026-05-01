#include "App.h"
#include "Bootloader.h"
#include "Button.h"
#include "Debug.h"
#include "Led.h"

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* Time the bootloader waits before auto-booting Slot A. */
#define WAIT_WINDOW_MS  5000U

/* How long the user must hold the button to select Slot B. */
#define BUTTON_HOLD_MS  3000U

/* LED blink period during the wait window. */
#define BLINK_PERIOD_MS 200U

void App_Start()
{
    Led_Init();
    Button_Init();

    Debug("Bootloader started. Hold button for %lu ms to boot Slot B.\n",
          (unsigned long)BUTTON_HOLD_MS);

    uint32_t window_start = HAL_GetTick();

    while ((HAL_GetTick() - window_start) < WAIT_WINDOW_MS)
    {
        Led_Blink(BLINK_PERIOD_MS);

        if (Button_IsHeldFor(BUTTON_HOLD_MS))
        {
            Debug("Button held - booting Slot B.\n");
            Led_Off();

            if (Bootloader_Verify_Slot(Slot_B) == true)
            {
                Bootloader_Jump_To_App(Slot_B);
            }

            /* Slot B invalid - fall back to default error state. */
            Bootloader_Default();
        }
    }

    /* Wait window expired - boot Slot A. */
    Debug("Timeout - booting Slot A.\n");
    Led_Off();

    if (Bootloader_Verify_Slot(Slot_A) == true)
    {
        Bootloader_Jump_To_App(Slot_A);
    }

    Bootloader_Default();
}
