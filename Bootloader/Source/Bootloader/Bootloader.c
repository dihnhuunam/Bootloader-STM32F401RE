/**
 * @file Bootloader.c
 * @brief Bootloader slot selection, OTA request commit, rollback, and jump implementation.
 */
#include "Bootloader.h"
#include "Debug.h"
#include "Flash.h"
#include "Image.h"
#include "Image_Config.h"
#include "Led.h"
#include "Button.h"
#include "crc32.h"
#include "usart.h"

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define BOOTLOADER_BOOT_MODE_HOLD_TIME_MS 3000U
#define BOOTLOADER_BUTTON_POLL_INTERVAL_MS 10U

/**
 * @brief Gets the fixed Flash layout information for a firmware slot.
 * @param slot Slot to describe.
 * @param
 * slot_info Output slot layout information.
 * @return true if the slot is valid and slot_info was filled.
 * @return
 * false if slot_info is NULL or the slot is invalid.
 */
static bool Bootloader_Get_Slot_Info(Bootloader_Slot_t slot, Bootloader_Slot_Info_t *slot_info)
{
    if (slot_info == NULL)
    {
        return false;
    }

    switch (slot)
    {
    case Slot_A:
        slot_info->base_addr = SLOT_A_BASE_ADDR;
        slot_info->end_addr = SLOT_A_END_ADDR;
        slot_info->vector_table_addr = SLOT_A_VECTOR_TABLE_ADDR;
        return true;

    case Slot_B:
        slot_info->base_addr = SLOT_B_BASE_ADDR;
        slot_info->end_addr = SLOT_B_END_ADDR;
        slot_info->vector_table_addr = SLOT_B_VECTOR_TABLE_ADDR;
        return true;

    default:
        return false;
    }
}

/**
 * @brief Calculates the CRC32 of a slot image payload.
 * @param header Image header stored at the slot base
 * address.
 * @param slot_info Slot layout information.
 * @param calculated_crc32 Output calculated CRC32 value.
 *
 * @return true if the image size is valid and CRC calculation succeeds.
 * @return false if arguments are invalid,
 * image size is invalid, or CRC calculation fails.
 */
static bool Bootloader_Calculate_Slot_Crc32(const ImageHeader_t *header, const Bootloader_Slot_Info_t *slot_info,
                                            uint32_t *calculated_crc32)
{
    Crc32_Status_t crc_status;
    uint32_t max_image_size;

    if ((header == NULL) || (slot_info == NULL) || (calculated_crc32 == NULL))
    {
        return false;
    }

    max_image_size = slot_info->end_addr - slot_info->vector_table_addr;
    if ((header->image_size == 0U) || (header->image_size > max_image_size))
    {
        Debug("Image size invalid: %lu\n", (unsigned long)header->image_size);
        return false;
    }

    crc_status = Crc32_Calculate_From_Flash(slot_info->vector_table_addr, header->image_size, calculated_crc32);
    if (crc_status != CRC32_STATUS_OK)
    {
        Debug("CRC32 calculate failed: status=%d\n", (int)crc_status);
        return false;
    }

    return true;
}

/**
 * @brief Checks whether a metadata slot value maps to a supported firmware slot.
 * @param slot Metadata slot value
 * to validate.
 * @return true if the slot is IMAGE_SLOT_A or IMAGE_SLOT_B.
 * @return false otherwise.
 */
static bool Bootloader_Is_Metadata_Slot_Valid(uint8_t slot)
{
    return (slot == IMAGE_SLOT_A) || (slot == IMAGE_SLOT_B);
}

/**
 * @brief Converts a metadata slot value to a bootloader slot enum.
 * @param slot Metadata slot value.
 * @return
 * Slot_B when slot is IMAGE_SLOT_B, otherwise Slot_A.
 */
static Bootloader_Slot_t Bootloader_Metadata_Slot_To_Bootloader_Slot(uint8_t slot)
{
    return (slot == IMAGE_SLOT_B) ? Slot_B : Slot_A;
}

/**
 * @brief Converts a bootloader slot enum to the metadata slot value.
 * @param slot Bootloader slot enum.
 *
 * @return IMAGE_SLOT_B for Slot_B, otherwise IMAGE_SLOT_A.
 */
static uint8_t Bootloader_Slot_To_Metadata_Slot(Bootloader_Slot_t slot)
{
    return (slot == Slot_B) ? IMAGE_SLOT_B : IMAGE_SLOT_A;
}

/**
 * @brief Gets the opposite firmware slot.
 * @param slot Current slot.
 * @return Slot_A when input is Slot_B,
 * otherwise Slot_B.
 */
static Bootloader_Slot_t Bootloader_Get_Other_Slot(Bootloader_Slot_t slot)
{
    return (slot == Slot_B) ? Slot_A : Slot_B;
}

/**
 * @brief Validates the persisted metadata header and active slot.
 * @param metadata Metadata object copied from
 * Flash.
 * @return true if metadata has the expected magic and valid active slot.
 * @return false otherwise.
 */
static bool Bootloader_Is_Metadata_Valid(const ImageMetadata_t *metadata)
{
    return (metadata != NULL) && (metadata->magic == METADATA_MAGIC_NUMBER) &&
           Bootloader_Is_Metadata_Slot_Valid(metadata->active_slot);
}

/**
 * @brief Copies boot metadata from Flash into RAM.
 * @param metadata Destination metadata buffer.
 */
static void Bootloader_Read_Metadata(ImageMetadata_t *metadata)
{
    if (metadata != NULL)
    {
        memcpy(metadata, (const void *)METADATA_BASE_ADDR, sizeof(*metadata));
    }
}

/**
 * @brief Erases and writes the metadata record to Flash.
 * @param metadata Metadata object to persist.
 * @return
 * true if erase and write completed successfully.
 * @return false otherwise.
 */
static bool Bootloader_Write_Metadata(const ImageMetadata_t *metadata)
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
 * @brief Verifies a slot and returns it as the selected boot slot.
 * @param candidate Slot to verify.
 * @param
 * slot Output selected slot when verification succeeds.
 * @return true if candidate verifies successfully.
 * @return
 * false otherwise.
 */
static bool Bootloader_Select_Verified_Slot(Bootloader_Slot_t candidate, Bootloader_Slot_t *slot)
{
    if ((slot != NULL) && Bootloader_Verify_Slot(candidate))
    {
        *slot = candidate;
        return true;
    }

    return false;
}

/**
 * @brief Initializes persistent metadata for a verified slot.
 * @param slot Slot that has already passed image
 * verification.
 *
 * The initialized metadata marks the slot as confirmed and uses the image
 * header CRC/version for
 * the matching slot record.
 */
static void Bootloader_Init_Metadata_For_Slot(Bootloader_Slot_t slot)
{
    Bootloader_Slot_Info_t slot_info;
    const ImageHeader_t *header;
    ImageMetadata_t metadata = {
        .magic = METADATA_MAGIC_NUMBER,
        .slot_a_crc32 = 0xFFFFFFFFUL,
        .slot_b_crc32 = 0xFFFFFFFFUL,
        .firmware_version = 0xFFFFFFFFUL,
        .active_slot = Bootloader_Slot_To_Metadata_Slot(slot),
        .rollback_slot = IMAGE_SLOT_INVALID,
        .pending_slot = IMAGE_SLOT_INVALID,
        .boot_state = IMAGE_BOOT_STATE_CONFIRMED,
        .boot_attempts = 0U,
    };

    if (!Bootloader_Get_Slot_Info(slot, &slot_info))
    {
        return;
    }

    header = (const ImageHeader_t *)slot_info.base_addr;
    metadata.firmware_version = header->version;

    if (slot == Slot_A)
    {
        metadata.slot_a_crc32 = header->crc32;
    }
    else
    {
        metadata.slot_b_crc32 = header->crc32;
    }

    if (Bootloader_Write_Metadata(&metadata))
    {
        Debug("Metadata initialized for Slot %c\n", (slot == Slot_A) ? 'A' : 'B');
    }
}

/**
 * @brief Selects a verified rollback slot and updates metadata to confirmed state.
 * @param metadata Metadata
 * object to update and persist. May be NULL to only select the slot.
 * @param rollback_slot Slot to verify and boot.

 * * @param slot Output selected slot when rollback succeeds.
 * @return true if rollback slot verifies successfully.
 *
 * @return false otherwise.
 */
static bool Bootloader_Rollback_To_Slot(ImageMetadata_t *metadata, Bootloader_Slot_t rollback_slot,
                                        Bootloader_Slot_t *slot)
{
    if (!Bootloader_Select_Verified_Slot(rollback_slot, slot))
    {
        return false;
    }

    if (metadata != NULL)
    {
        metadata->active_slot = Bootloader_Slot_To_Metadata_Slot(rollback_slot);
        metadata->pending_slot = IMAGE_SLOT_INVALID;
        metadata->rollback_slot = IMAGE_SLOT_INVALID;
        metadata->boot_state = IMAGE_BOOT_STATE_CONFIRMED;
        metadata->boot_attempts = 0U;
        (void)Bootloader_Write_Metadata(metadata);
    }

    Debug("Rollback slot selected\n");
    return true;
}

static void Bootloader_Read_Ota_Request(OtaRequest_t *request)
{
    if (request != NULL)
    {
        memcpy(request, (const void *)OTA_REQUEST_BASE_ADDR, sizeof(*request));
    }
}

static void Bootloader_Clear_Ota_Request(void)
{
    if (Flash_Erase(OTA_REQUEST_BASE_ADDR, OTA_REQUEST_SIZE) != FLASH_STATUS_OK)
    {
        Debug("OTA request erase failed: 0x%08lX\n", (unsigned long)Flash_GetLastError());
    }
}

static bool Bootloader_Is_Ota_Request_Header_Valid(const OtaRequest_t *request)
{
    return (request != NULL) && (request->magic == OTA_REQUEST_MAGIC) &&
           Bootloader_Is_Metadata_Slot_Valid(request->target_slot);
}

static bool Bootloader_Commit_Ota_Request(const OtaRequest_t *request)
{
    ImageMetadata_t metadata;
    Bootloader_Slot_t active_slot;
    Bootloader_Slot_t target_slot;
    Bootloader_Slot_Info_t target_info;
    const ImageHeader_t *header;

    if (!Bootloader_Is_Ota_Request_Header_Valid(request))
    {
        return false;
    }

    Bootloader_Read_Metadata(&metadata);
    if (!Bootloader_Is_Metadata_Valid(&metadata))
    {
        Debug("OTA request ignored: metadata invalid\n");
        Bootloader_Clear_Ota_Request();
        return false;
    }

    target_slot = Bootloader_Metadata_Slot_To_Bootloader_Slot(request->target_slot);
    active_slot = Bootloader_Metadata_Slot_To_Bootloader_Slot(metadata.active_slot);

    if (!Bootloader_Get_Slot_Info(target_slot, &target_info))
    {
        Bootloader_Clear_Ota_Request();
        return false;
    }

    header = (const ImageHeader_t *)target_info.base_addr;
    if ((header->crc32 != request->image_crc32) || (header->image_size != request->image_size))
    {
        Debug("OTA request mismatch\n");
        Bootloader_Clear_Ota_Request();
        return false;
    }

    if (!Bootloader_Verify_Slot(target_slot))
    {
        Debug("OTA request target is not bootable\n");
        Bootloader_Clear_Ota_Request();
        return false;
    }

    if (target_slot == active_slot)
    {
        Debug("OTA request target is already active\n");
        Bootloader_Clear_Ota_Request();
        return true;
    }

    metadata.pending_slot = Bootloader_Slot_To_Metadata_Slot(target_slot);
    metadata.rollback_slot = Bootloader_Slot_To_Metadata_Slot(active_slot);
    metadata.boot_state = IMAGE_BOOT_STATE_PENDING_CONFIRM;
    metadata.boot_attempts = 0U;
    metadata.firmware_version = header->version;

    if (target_slot == Slot_A)
    {
        metadata.slot_a_crc32 = header->crc32;
    }
    else
    {
        metadata.slot_b_crc32 = header->crc32;
    }

    if (!Bootloader_Write_Metadata(&metadata))
    {
        return false;
    }

    Bootloader_Clear_Ota_Request();
    Debug("OTA request committed for Slot %c\n", (target_slot == Slot_A) ? 'A' : 'B');
    return true;
}

static void Bootloader_Process_Ota_Request(void)
{
    OtaRequest_t request;

    Bootloader_Read_Ota_Request(&request);
    if (request.magic == 0xFFFFFFFFUL)
    {
        return;
    }

    if (!Bootloader_Is_Ota_Request_Header_Valid(&request))
    {
        Debug("Invalid OTA request\n");
        Bootloader_Clear_Ota_Request();
        return;
    }

    (void)Bootloader_Commit_Ota_Request(&request);
}

static bool Bootloader_Is_Stack_Pointer_Valid(uint32_t stack_pointer)
{
    return (stack_pointer >= SRAM_START_ADDR) && (stack_pointer <= SRAM_END_ADDR) && ((stack_pointer & 0x7U) == 0U);
}

static bool Bootloader_Is_Reset_Handler_Valid(uint32_t reset_handler, const Bootloader_Slot_Info_t *slot_info)
{
    return (slot_info != NULL) && (reset_handler >= slot_info->vector_table_addr) &&
           (reset_handler < slot_info->end_addr) && ((reset_handler & 0x1U) != 0U);
}

/**
 * @brief Checks whether the user button is currently pressed.
 * @return true if PC13 reads low.
 * @return false otherwise.
 */
static bool Bootloader_Is_User_Button_Pressed(void)
{
    return Button_Is_Pressed();
}

/**
 * @brief Receives one CLI command byte from the debug UART.
 * @param command Output received command byte.
 * @return true if a byte was received.
 * @return false otherwise.
 */
static bool Bootloader_Cli_Read_Command(uint8_t *command)
{
    return (command != NULL) && (HAL_UART_Receive(&huart2, command, 1U, HAL_MAX_DELAY) == HAL_OK);
}

/**
 * @brief Verifies whether a firmware slot contains a bootable image.
 * @param slot Slot to verify.
 * @return true
 * if image header, CRC, stack pointer, and reset handler are valid.
 * @return false if the slot is invalid or
 * verification fails.
 */
bool Bootloader_Verify_Slot(Bootloader_Slot_t slot)
{
    Bootloader_Slot_Info_t slot_info;
    uint32_t calculated_crc32 = 0U;

    if (!Bootloader_Get_Slot_Info(slot, &slot_info))
    {
        Debug("Invalid boot slot\n");
        return false;
    }

    const ImageHeader_t *header = (const ImageHeader_t *)slot_info.base_addr;

    if (header->magic != APP_MAGIC_NUMBER)
    {
        Debug("Magic number incorrect!\n");
        return false;
    }

    if (!Bootloader_Calculate_Slot_Crc32(header, &slot_info, &calculated_crc32))
    {
        return false;
    }

    if (header->crc32 != calculated_crc32)
    {
        Debug("CRC32 incorrect: header=0x%08lX calc=0x%08lX\n", (unsigned long)header->crc32,
              (unsigned long)calculated_crc32);
        return false;
    }

    // Verify Stack Pointer
    const VectorTable_t *vtable = (const VectorTable_t *)slot_info.vector_table_addr;
    if (!Bootloader_Is_Stack_Pointer_Valid(vtable->stack_pointer))
    {
        Debug("Stack pointer is invalid\n");
        return false;
    }

    if (!Bootloader_Is_Reset_Handler_Valid(vtable->reset_handler, &slot_info))
    {
        Debug("Reset handler is invalid\n");
        return false;
    }

    // Application Verified Successfully
    Debug("Application verified\n");
    return true;
}

/**
 * @brief Selects the firmware slot to boot according to metadata state.
 * @param slot Output selected slot.
 *
 * @return true if a bootable slot was selected.
 * @return false if no valid slot is available.
 */
bool Bootloader_Select_Boot_Slot(Bootloader_Slot_t *slot)
{
    ImageMetadata_t metadata;
    Bootloader_Slot_t active_slot;
    Bootloader_Slot_t pending_slot;
    Bootloader_Slot_t rollback_slot;
    bool has_pending;

    if (slot == NULL)
    {
        return false;
    }

    Bootloader_Process_Ota_Request();

    Bootloader_Read_Metadata(&metadata);
    if (!Bootloader_Is_Metadata_Valid(&metadata))
    {
        Debug("Metadata invalid, fallback to available slot\n");
        if (Bootloader_Select_Verified_Slot(Slot_A, slot))
        {
            Bootloader_Init_Metadata_For_Slot(Slot_A);
            return true;
        }

        if (Bootloader_Select_Verified_Slot(Slot_B, slot))
        {
            Bootloader_Init_Metadata_For_Slot(Slot_B);
            return true;
        }

        return false;
    }

    active_slot = Bootloader_Metadata_Slot_To_Bootloader_Slot(metadata.active_slot);
    has_pending = (metadata.boot_state == IMAGE_BOOT_STATE_PENDING_CONFIRM) &&
                  Bootloader_Is_Metadata_Slot_Valid(metadata.pending_slot);

    if (has_pending)
    {
        pending_slot = Bootloader_Metadata_Slot_To_Bootloader_Slot(metadata.pending_slot);

        if (metadata.boot_attempts >= METADATA_MAX_BOOT_ATTEMPTS)
        {
            Debug("Pending image exceeded boot attempts\n");
            rollback_slot = Bootloader_Is_Metadata_Slot_Valid(metadata.rollback_slot)
                                ? Bootloader_Metadata_Slot_To_Bootloader_Slot(metadata.rollback_slot)
                                : active_slot;
            return Bootloader_Rollback_To_Slot(&metadata, rollback_slot, slot);
        }

        if (Bootloader_Verify_Slot(pending_slot))
        {
            metadata.boot_attempts++;
            (void)Bootloader_Write_Metadata(&metadata);
            *slot = pending_slot;
            return true;
        }

        rollback_slot = Bootloader_Is_Metadata_Slot_Valid(metadata.rollback_slot)
                            ? Bootloader_Metadata_Slot_To_Bootloader_Slot(metadata.rollback_slot)
                            : active_slot;
        return Bootloader_Rollback_To_Slot(&metadata, rollback_slot, slot);
    }

    if ((metadata.boot_state != IMAGE_BOOT_STATE_ROLLBACK) && Bootloader_Select_Verified_Slot(active_slot, slot))
    {
        return true;
    }

    if (Bootloader_Is_Metadata_Slot_Valid(metadata.rollback_slot))
    {
        rollback_slot = Bootloader_Metadata_Slot_To_Bootloader_Slot(metadata.rollback_slot);
        if (rollback_slot != active_slot)
        {
            return Bootloader_Rollback_To_Slot(&metadata, rollback_slot, slot);
        }
    }

    rollback_slot = Bootloader_Get_Other_Slot(active_slot);
    if (Bootloader_Select_Verified_Slot(rollback_slot, slot))
    {
        metadata.active_slot = Bootloader_Slot_To_Metadata_Slot(rollback_slot);
        metadata.pending_slot = IMAGE_SLOT_INVALID;
        metadata.rollback_slot = IMAGE_SLOT_INVALID;
        metadata.boot_state = IMAGE_BOOT_STATE_CONFIRMED;
        metadata.boot_attempts = 0U;
        (void)Bootloader_Write_Metadata(&metadata);
        return true;
    }

    return false; /* Safe mode/recovery will be implemented later. */
}

/**
 * @brief Checks whether Boot Mode should be entered.
 * @return true if the user button is held for the required duration.
 * @return false otherwise.
 */
bool Bootloader_Should_Enter_Boot_Mode(void)
{
    uint32_t held_time_ms = 0U;

    if (!Bootloader_Is_User_Button_Pressed())
    {
        return false;
    }
    Led_On();
    Debug("Hold USER button for 3 seconds to enter Boot Mode...\n");
    while (held_time_ms < BOOTLOADER_BOOT_MODE_HOLD_TIME_MS)
    {
        if (!Bootloader_Is_User_Button_Pressed())
        {
            Debug("Boot Mode canceled\n");
            return false;
        }

        HAL_Delay(BOOTLOADER_BUTTON_POLL_INTERVAL_MS);
        held_time_ms += BOOTLOADER_BUTTON_POLL_INTERVAL_MS;
    }

    Debug("Entering Boot Mode\n");
    return true;
}

/**
 * @brief Runs the manual Boot Mode CLI.
 *
 * The CLI lets the user select Slot A or Slot B manually. If the selected
 * slot verifies successfully, execution is transferred to that slot.
 */
void Bootloader_Run_Boot_Mode(void)
{
    uint8_t command;
    Led_Off();
    while (1)
    {
        Debug("\n=== Boot Mode ===\n");
        Debug("1/a: Boot Slot A\n");
        Debug("2/b: Boot Slot B\n");
        Debug("r  : Resume normal boot\n");
        Debug("> ");
        HAL_Delay(10U);
        if (!Bootloader_Cli_Read_Command(&command))
        {
            Debug("\nCLI read failed\n");
            continue;
        }

        Debug("%c\n", command);

        if ((command == '1') || (command == 'a') || (command == 'A'))
        {
            if (Bootloader_Verify_Slot(Slot_A))
            {
                Bootloader_Jump_To_App(Slot_A);
            }
            Debug("Slot A is not bootable\n");
        }
        else if ((command == '2') || (command == 'b') || (command == 'B'))
        {
            if (Bootloader_Verify_Slot(Slot_B))
            {
                Bootloader_Jump_To_App(Slot_B);
            }
            Debug("Slot B is not bootable\n");
        }
        else if ((command == 'r') || (command == 'R'))
        {
            Debug("Resume normal boot\n");
            return;
        }
        else
        {
            Debug("Invalid command\n");
        }
    }
}

/**
 * @brief Transfers execution to the selected application slot.
 * @param slot Slot to jump to.
 */
void Bootloader_Jump_To_App(Bootloader_Slot_t slot)
{
    Bootloader_Slot_Info_t slot_info;

    if (!Bootloader_Get_Slot_Info(slot, &slot_info))
    {
        Debug("Invalid boot slot\n");
        return;
    }

    // Get Stack Pointer * Reset Handler address from Application Vector Table
    const VectorTable_t *vtable = (const VectorTable_t *)slot_info.vector_table_addr;
    uint32_t app_stack_pointer_addr = vtable->stack_pointer;
    uint32_t app_reset_handler_addr = vtable->reset_handler;

    // Function pointer to Reset_Handler
    void (*app_reset_handler)(void);
    app_reset_handler = (void *)app_reset_handler_addr;

    // Disable all interrupts
    __disable_irq();

    // Deregister SysTick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    // Remove pending interrupts
    for (int i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF; /* Disable */
        NVIC->ICPR[i] = 0xFFFFFFFF; /* Clear pending */
    }

    // Set Application's Vector Table
    SCB->VTOR = slot_info.vector_table_addr;
    __DSB();
    __ISB();

    // Set Stack Pointer
    __set_MSP(app_stack_pointer_addr);

    // Let the application initialize its own tick and peripheral interrupts.
    __enable_irq();

    // Start Application's Reset Handler
    app_reset_handler();
}

/**
 * @brief Enters the default bootloader fallback loop.
 */
void Bootloader_Default()
{
    Debug("Bootloader default\n");
    while (1)
    {
        Led_Blink(200);
    }
}
