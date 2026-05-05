#include "App.h"
#include "Debug.h"
#include "Flash.h"
#include "Image.h"
#include "Image_Config.h"
#include "Led.h"
#include "stm32f4xx_hal.h"

#include <stdbool.h>
#include <string.h>

const ImageHeader_t __app_header __attribute__((section(".app_header"), used)) = {
    .magic = APP_MAGIC_NUMBER, .version = (0x00010000), .crc32 = 0x00000000, .image_size = 0x00000000};

static bool App_Metadata_Is_Valid(const ImageMetadata_t *metadata)
{
    return (metadata != NULL) && (metadata->magic == METADATA_MAGIC_NUMBER) &&
           ((metadata->active_slot == IMAGE_SLOT_A) || (metadata->active_slot == IMAGE_SLOT_B));
}

static bool App_Metadata_Write(const ImageMetadata_t *metadata)
{
    if (metadata == NULL)
    {
        return false;
    }

    if (Flash_Erase(METADATA_BASE_ADDR, sizeof(*metadata)) != FLASH_STATUS_OK)
    {
        Debug("Metadata erase failed: 0x%08lX\n", (unsigned long)Flash_GetLastError());
        return false;
    }

    if (Flash_Write(METADATA_BASE_ADDR, (const uint8_t *)metadata, sizeof(*metadata)) != FLASH_STATUS_OK)
    {
        Debug("Metadata write failed: 0x%08lX\n", (unsigned long)Flash_GetLastError());
        return false;
    }

    return true;
}

static void App_Confirm_Boot(void)
{
    ImageMetadata_t metadata;
    bool is_active_slot;
    bool is_pending_slot;

    memcpy(&metadata, (const void *)METADATA_BASE_ADDR, sizeof(metadata));
    if (!App_Metadata_Is_Valid(&metadata))
    {
        return;
    }

    is_active_slot = metadata.active_slot == IMAGE_SLOT_B;
    is_pending_slot =
        (metadata.boot_state == IMAGE_BOOT_STATE_PENDING_CONFIRM) && (metadata.pending_slot == IMAGE_SLOT_B);

    if (!is_active_slot && !is_pending_slot)
    {
        return;
    }

    if (is_active_slot && (metadata.boot_state == IMAGE_BOOT_STATE_CONFIRMED) &&
        (metadata.pending_slot == IMAGE_SLOT_INVALID) && (metadata.boot_attempts == 0U))
    {
        return;
    }

    metadata.active_slot = IMAGE_SLOT_B;
    metadata.slot_b_crc32 = __app_header.crc32;
    metadata.firmware_version = __app_header.version;
    metadata.pending_slot = IMAGE_SLOT_INVALID;
    metadata.rollback_slot = IMAGE_SLOT_INVALID;
    metadata.boot_state = IMAGE_BOOT_STATE_CONFIRMED;
    metadata.boot_attempts = 0U;

    if (App_Metadata_Write(&metadata))
    {
        Debug("Application 2 boot confirmed\n");
    }
}

void App_Start()
{
    App_Confirm_Boot();

    while (1)
    {
        Led_Blink(3000);
        Debug("Application Firmware 2\n");
        HAL_Delay(3000);
    }
}
