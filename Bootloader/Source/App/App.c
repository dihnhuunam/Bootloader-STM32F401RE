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

/**
 * Verifies a slot and jumps to it if valid.  Returns only if verification
 * fails; Bootloader_Jump_To_App() never returns on success.
 */
static void App_Try_Boot(Bootloader_Slot_t slot)
{
    if (Bootloader_Verify_Slot(slot) == true)
    {
        Bootloader_Jump_To_App(slot);
    }
}

#ifdef DEMO_ROLLBACK
/* Blocks for (on_ms + off_ms) * count ms, producing a distinct blink pattern. */
static void App_Demo_Blink(uint32_t on_ms, uint32_t off_ms, uint32_t count)
{
    for (uint32_t i = 0U; i < count; i++)
    {
        Led_On();
        HAL_Delay(on_ms);
        Led_Off();
        HAL_Delay(off_ms);
    }
}
#endif /* DEMO_ROLLBACK */

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
            Led_Off();
            Debug("Button held - attempting Slot B.\n");

#ifdef DEMO_ROLLBACK
            /* 3 short blinks: bootloader is about to try Slot B. */
            App_Demo_Blink(100U, 100U, 3U);
#endif

            App_Try_Boot(Slot_B);

            /* Slot B verification failed - roll back to Slot A. */
            Debug("Slot B invalid. Rolling back to Slot A.\n");

#ifdef DEMO_ROLLBACK
            /* LED solid for 1 s: Slot B has failed. */
            Led_On();
            HAL_Delay(1000U);
            Led_Off();
            HAL_Delay(200U);

            /* 5 rapid blinks: rollback in progress. */
            App_Demo_Blink(50U, 50U, 5U);
#endif

            App_Try_Boot(Slot_A);

            /* Both slots failed. */
            Bootloader_Default();
        }
    }

    /* Wait window expired - boot Slot A. */
    Debug("Timeout - booting Slot A.\n");
    Led_Off();
    App_Try_Boot(Slot_A);
    Bootloader_Default();
}
