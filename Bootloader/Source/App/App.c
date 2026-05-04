#include "App.h"
#include "Bootloader.h"
#include "Debug.h"
#include "UartFirmwareUpdate.h"
#include "usart.h"

#include <stdbool.h>

void App_Start()
{
    UartFirmwareUpdate_Status_t update_status;
    const Bootloader_Slot_t update_slot = Slot_B;

    update_status = UartFirmwareUpdate_Receive_To_Slot(&huart1, update_slot);
    if (update_status == UART_FW_UPDATE_STATUS_OK)
    {
        Debug("UART firmware update written\n");
        if (Bootloader_Verify_Slot(update_slot) == true)
        {
            Bootloader_Jump_To_App(update_slot);
        }

        Debug("Updated firmware invalid after UART update\n");
    }
    else if (update_status != UART_FW_UPDATE_STATUS_NO_UPDATE)
    {
        Debug("UART firmware update failed: %d\n", (int)update_status);
    }

    if (Bootloader_Verify_Slot(update_slot) == true)
    {
        Bootloader_Jump_To_App(update_slot);
    }
    else
    {
        Bootloader_Default();
    }
}
