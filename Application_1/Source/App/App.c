#include "App.h"
#include "Debug.h"
#include "Flash.h"
#include "Image.h"
#include "Image_Config.h"
#include "Led.h"
#include "UartOTA.h"
#include "stm32f4xx_hal.h"

#include <stdbool.h>
#include <string.h>

const ImageHeader_t __app_header __attribute__((section(".app_header"), used)) = {
    .magic = APP_MAGIC_NUMBER, .version = 0x00000000, .crc32 = 0x00000000, .image_size = 0x00000000};

/**
 * @brief Validates the boot metadata header and active slot field.
 * @param metadata Metadata object copied from Flash.
 * @return true if metadata has the expected magic and valid active slot.
 * @return false otherwise.
 */
static bool App_Metadata_Is_Valid(const ImageMetadata_t *metadata)
{
    return (metadata != NULL) && (metadata->magic == METADATA_MAGIC_NUMBER) &&
           ((metadata->active_slot == IMAGE_SLOT_A) || (metadata->active_slot == IMAGE_SLOT_B));
}

/**
 * @brief Erases and writes the metadata record to Flash.
 * @param metadata Metadata object to persist.
 * @return true if erase and write completed successfully.
 * @return false otherwise.
 */
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

/**
 * @brief Confirms Application 1 as the successfully booted image.
 *
 * Slot A is confirmed when it is already the active slot or when it is the
 * pending slot selected by the bootloader. Confirmation updates metadata to
 * mark Slot A as active, clears pending/rollback state, and resets boot attempts.
 */
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

    is_active_slot = metadata.active_slot == IMAGE_SLOT_A;
    is_pending_slot =
        (metadata.boot_state == IMAGE_BOOT_STATE_PENDING_CONFIRM) && (metadata.pending_slot == IMAGE_SLOT_A);

    if (!is_active_slot && !is_pending_slot)
    {
        return;
    }

    if (is_active_slot && (metadata.boot_state == IMAGE_BOOT_STATE_CONFIRMED) &&
        (metadata.pending_slot == IMAGE_SLOT_INVALID) && (metadata.boot_attempts == 0U))
    {
        return;
    }

    metadata.active_slot = IMAGE_SLOT_A;
    metadata.slot_a_crc32 = __app_header.crc32;
    metadata.firmware_version = __app_header.version;
    metadata.pending_slot = IMAGE_SLOT_INVALID;
    metadata.rollback_slot = IMAGE_SLOT_INVALID;
    metadata.boot_state = IMAGE_BOOT_STATE_CONFIRMED;
    metadata.boot_attempts = 0U;

    if (App_Metadata_Write(&metadata))
    {
        Debug("Application 1 boot confirmed\n");
    }
}

/**
 * @brief Starts the application runtime.
 *
 * Confirms the current boot image in metadata when applicable, then enters
 * the main application loop.
 */
void App_Start()
{
    const ImageHeader_t *image_header = (const ImageHeader_t *)SLOT_A_BASE_ADDR;
    uint32_t version = image_header->version;
    uint32_t version_major = (version >> 16) & 0xFFFFU;
    uint32_t version_minor = (version >> 8) & 0xFFU;
    uint32_t version_patch = version & 0xFFU;

    App_Confirm_Boot();
    UartOTA_Init();

    while (1)
    {
        UartOTA_Process(IMAGE_SLOT_A);
        Led_Blink(3000);
        Debug("Application Firmware 1 v%lu.%lu.%lu\n", (unsigned long)version_major, (unsigned long)version_minor,
              (unsigned long)version_patch);
        HAL_Delay(3000);
    }
}
