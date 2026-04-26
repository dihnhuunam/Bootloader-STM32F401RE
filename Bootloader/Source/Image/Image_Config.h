#ifndef IMAGE_CONFIG_H
#define IMAGE_CONFIG_H

/**
 * @file Image_Config.h
 * @brief Flash layout and firmware image configuration for STM32F401.
 *
 * This file defines the flash memory layout used by the bootloader,
 * metadata region, and dual application slots.
 *
 * The address ranges use the convention:
 * - `BASE_ADDR` is inclusive.
 * - `END_ADDR` is exclusive, meaning it points to the first address
 *   after the valid region.
 *
 * Example:
 * @code
 * Valid range: [SLOT_A_BASE_ADDR, SLOT_A_END_ADDR)
 * @endcode
 *
 * Therefore, address validation should use:
 * @code
 * addr >= BASE_ADDR && addr < END_ADDR
 * @endcode
 */

/**
 * @defgroup flash_layout STM32F401 Flash Layout
 * @brief Flash memory map for 512 KB internal flash.
 *
 * STM32F401 flash layout:
 *
 * | Region      | Sectors        | Size   | Address Range              |
 * |------------|----------------|--------|-----------------------------|
 * | Bootloader | Sector 0 - 1   | 32 KB  | 0x08000000 - 0x08007FFF     |
 * | Metadata   | Sector 2 - 3   | 32 KB  | 0x08008000 - 0x0800FFFF     |
 * | App Slot A | Sector 4 - 5   | 192 KB | 0x08010000 - 0x0803FFFF     |
 * | App Slot B | Sector 6 - 7   | 256 KB | 0x08040000 - 0x0807FFFF     |
 *
 * @{
 */

/* -------------------------------------------------------------------------- */
/* Bootloader region                                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Base address of the bootloader region.
 *
 * The bootloader starts at the beginning of internal flash.
 */
#define BOOTLOADER_BASE_ADDR 0x08000000UL

/**
 * @brief Size of the bootloader region in bytes.
 *
 * The bootloader occupies Sector 0 and Sector 1.
 */
#define BOOTLOADER_SIZE 0x00008000UL

/**
 * @brief Exclusive end address of the bootloader region.
 *
 * Valid bootloader addresses are in the range:
 * @code
 * [BOOTLOADER_BASE_ADDR, BOOTLOADER_END_ADDR)
 * @endcode
 */
#define BOOTLOADER_END_ADDR (BOOTLOADER_BASE_ADDR + BOOTLOADER_SIZE)

/* -------------------------------------------------------------------------- */
/* Metadata region                                                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Base address of the metadata region.
 *
 * The metadata region stores firmware validation data and active slot state.
 */
#define METADATA_BASE_ADDR 0x08008000UL

/**
 * @brief Size of the metadata region in bytes.
 *
 * The metadata region occupies Sector 2 and Sector 3.
 */
#define METADATA_SIZE 0x00008000UL

/**
 * @brief Exclusive end address of the metadata region.
 *
 * Valid metadata addresses are in the range:
 * @code
 * [METADATA_BASE_ADDR, METADATA_END_ADDR)
 * @endcode
 */
#define METADATA_END_ADDR (METADATA_BASE_ADDR + METADATA_SIZE)

/**
 * @brief Offset of the metadata magic word.
 *
 * Size: 4 bytes.
 */
#define METADATA_MAGIC_OFFSET 0x0000U

/**
 * @brief Offset of the stored CRC32 value for Application Slot A.
 *
 * Size: 4 bytes.
 */
#define METADATA_CRC_APP1_OFFSET 0x0004U

/**
 * @brief Offset of the stored CRC32 value for Application Slot B.
 *
 * Size: 4 bytes.
 */
#define METADATA_CRC_APP2_OFFSET 0x0008U

/**
 * @brief Offset of the firmware version field.
 *
 * Size: 4 bytes.
 *
 * Example:
 * @code
 * 0x0102U // Version 1.2
 * @endcode
 */
#define METADATA_FW_VERSION_OFFSET 0x000CU

/**
 * @brief Offset of the active slot selection field.
 *
 * Size: 1 byte.
 *
 * Values:
 * - 0: Application Slot A is active.
 * - 1: Application Slot B is active.
 */
#define METADATA_ACTIVE_SLOT_OFFSET 0x0010U

/**
 * @brief Magic value used to mark metadata as valid.
 */
#define METADATA_MAGIC_NUMBER 0xA5A5A5A5UL

/* -------------------------------------------------------------------------- */
/* Application Slot A                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Base address of Application Slot A.
 *
 * Slot A is the primary application slot.
 */
#define SLOT_A_BASE_ADDR 0x08010000UL

/**
 * @brief Size of Application Slot A in bytes.
 *
 * Slot A occupies Sector 4 and Sector 5.
 */
#define SLOT_A_SIZE 0x00030000UL

/**
 * @brief Exclusive end address of Application Slot A.
 *
 * Valid Slot A addresses are in the range:
 * @code
 * [SLOT_A_BASE_ADDR, SLOT_A_END_ADDR)
 * @endcode
 */
#define SLOT_A_END_ADDR (SLOT_A_BASE_ADDR + SLOT_A_SIZE)

/**
 * @brief Offset from the slot base to the application vector table.
 *
 * The first 512 bytes of each application slot are reserved for the
 * application header, including magic, CRC, firmware size, and version.
 */
#define VTABLE_OFFSET 0x200U

/**
 * @brief Address of the vector table for Application Slot A.
 */
#define SLOT_A_VECTOR_TABLE_ADDR (SLOT_A_BASE_ADDR + VTABLE_OFFSET)

/**
 * @brief Address of the magic field in the Slot A application header.
 *
 * Size: 4 bytes.
 */
#define SLOT_A_MAGIC_ADDR (SLOT_A_BASE_ADDR + 0x00U)

/**
 * @brief Address of the CRC32 field in the Slot A application header.
 *
 * Size: 4 bytes.
 */
#define SLOT_A_CRC_ADDR (SLOT_A_BASE_ADDR + 0x04U)

/**
 * @brief Address of the firmware size field in the Slot A application header.
 *
 * Size: 4 bytes.
 */
#define SLOT_A_FW_SIZE_ADDR (SLOT_A_BASE_ADDR + 0x08U)

/**
 * @brief Address of the firmware version field in the Slot A application header.
 *
 * Size: 4 bytes.
 */
#define SLOT_A_FW_VERSION_ADDR (SLOT_A_BASE_ADDR + 0x0CU)

/**
 * @brief Base address of executable code in Application Slot A.
 *
 * This address is also the vector table address.
 */
#define SLOT_A_CODE_BASE_ADDR SLOT_A_VECTOR_TABLE_ADDR

/**
 * @brief Size of the executable code region in Application Slot A.
 *
 * This excludes the reserved application header area.
 */
#define SLOT_A_CODE_SIZE (SLOT_A_SIZE - VTABLE_OFFSET)

/* -------------------------------------------------------------------------- */
/* Application Slot B                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Base address of Application Slot B.
 *
 * Slot B is the secondary application slot, typically used for OTA update
 * or A/B firmware switching.
 */
#define SLOT_B_BASE_ADDR 0x08040000UL

/**
 * @brief Size of Application Slot B in bytes.
 *
 * Slot B occupies Sector 6 and Sector 7.
 */
#define SLOT_B_SIZE 0x00040000UL

/**
 * @brief Exclusive end address of Application Slot B.
 *
 * Valid Slot B addresses are in the range:
 * @code
 * [SLOT_B_BASE_ADDR, SLOT_B_END_ADDR)
 * @endcode
 */
#define SLOT_B_END_ADDR (SLOT_B_BASE_ADDR + SLOT_B_SIZE)

/**
 * @brief Address of the vector table for Application Slot B.
 */
#define SLOT_B_VECTOR_TABLE_ADDR (SLOT_B_BASE_ADDR + VTABLE_OFFSET)

/**
 * @brief Address of the magic field in the Slot B application header.
 *
 * Size: 4 bytes.
 */
#define SLOT_B_MAGIC_ADDR (SLOT_B_BASE_ADDR + 0x00U)

/**
 * @brief Address of the CRC32 field in the Slot B application header.
 *
 * Size: 4 bytes.
 */
#define SLOT_B_CRC_ADDR (SLOT_B_BASE_ADDR + 0x04U)

/**
 * @brief Address of the firmware size field in the Slot B application header.
 *
 * Size: 4 bytes.
 */
#define SLOT_B_FW_SIZE_ADDR (SLOT_B_BASE_ADDR + 0x08U)

/**
 * @brief Address of the firmware version field in the Slot B application header.
 *
 * Size: 4 bytes.
 */
#define SLOT_B_FW_VERSION_ADDR (SLOT_B_BASE_ADDR + 0x0CU)

/**
 * @brief Base address of executable code in Application Slot B.
 *
 * This address is also the vector table address.
 */
#define SLOT_B_CODE_BASE_ADDR SLOT_B_VECTOR_TABLE_ADDR

/**
 * @brief Size of the executable code region in Application Slot B.
 *
 * This excludes the reserved application header area.
 */
#define SLOT_B_CODE_SIZE (SLOT_B_SIZE - VTABLE_OFFSET)

/* -------------------------------------------------------------------------- */
/* Flash constants                                                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Base address of STM32F401 internal flash memory.
 */
#define FLASH_BASE_ADDR 0x08000000UL

/**
 * @brief Total size of STM32F401 internal flash memory in bytes.
 */
#define FLASH_TOTAL_SIZE 0x00080000UL

/**
 * @brief Exclusive end address of STM32F401 internal flash memory.
 *
 * Valid flash addresses are in the range:
 * @code
 * [FLASH_BASE_ADDR, FLASH_END_ADDR)
 * @endcode
 */
#define FLASH_END_ADDR (FLASH_BASE_ADDR + FLASH_TOTAL_SIZE)

/* -------------------------------------------------------------------------- */
/* SRAM constants                                                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Base address of STM32F401 internal SRAM.
 */
#define SRAM_START_ADDR 0x20000000UL

/**
 * @brief Exclusive end address of STM32F401 internal SRAM.
 *
 * The STM32F401 has 96 KB of SRAM.
 */
#define SRAM_END_ADDR 0x20018000UL

/**
 * @brief Total SRAM size in bytes.
 */
#define SRAM_SIZE (SRAM_END_ADDR - SRAM_START_ADDR)

/* -------------------------------------------------------------------------- */
/* Application magic and CRC values                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Magic value used to mark an application image as valid.
 *
 * This value should be written into the application header at build time.
 */
#define APP_MAGIC_NUMBER 0xA5A5A5A5UL

/**
 * @brief Expected CRC32 value for Application Slot A.
 *
 * This value must be updated after each Slot A firmware build.
 */
#define SLOT_A_CRC_32 0x24C13CC6

/**
 * @brief Expected CRC32 value for Application Slot B.
 *
 * This value must be updated after each Slot B firmware build.
 */
#define SLOT_B_CRC_32 0x2B0DE2BE

/* -------------------------------------------------------------------------- */
/* Helper macros                                                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief Check whether an address is inside Application Slot A.
 *
 * @param addr Address to check.
 *
 * @return true if the address is within Slot A.
 * @return false otherwise.
 *
 * The valid range is:
 * @code
 * [SLOT_A_BASE_ADDR, SLOT_A_END_ADDR)
 * @endcode
 */
#define IS_ADDR_IN_SLOT_A(addr) (((addr) >= SLOT_A_BASE_ADDR) && ((addr) < SLOT_A_END_ADDR))

/**
 * @brief Check whether an address is inside Application Slot B.
 *
 * @param addr Address to check.
 *
 * @return true if the address is within Slot B.
 * @return false otherwise.
 *
 * The valid range is:
 * @code
 * [SLOT_B_BASE_ADDR, SLOT_B_END_ADDR)
 * @endcode
 */
#define IS_ADDR_IN_SLOT_B(addr) (((addr) >= SLOT_B_BASE_ADDR) && ((addr) < SLOT_B_END_ADDR))

/**
 * @brief Check whether an initial stack pointer value is valid.
 *
 * @param sp Stack pointer value read from the application vector table.
 *
 * @return true if the stack pointer points into SRAM.
 * @return false otherwise.
 *
 * The initial stack pointer must point to a valid SRAM address before
 * the bootloader jumps to the application.
 */
#define IS_VALID_STACK_POINTER(sp) (((sp) > SRAM_START_ADDR) && ((sp) <= SRAM_END_ADDR))

/**
 * @brief Check whether a Reset_Handler address is valid for Slot A.
 *
 * @param addr Reset_Handler address read from the Slot A vector table.
 *
 * @return true if the Reset_Handler address is inside Slot A.
 * @return false otherwise.
 */
#define IS_VALID_RESET_HANDLER_SLOT_A(addr) IS_ADDR_IN_SLOT_A(addr)

/**
 * @brief Check whether a Reset_Handler address is valid for Slot B.
 *
 * @param addr Reset_Handler address read from the Slot B vector table.
 *
 * @return true if the Reset_Handler address is inside Slot B.
 * @return false otherwise.
 */
#define IS_VALID_RESET_HANDLER_SLOT_B(addr) IS_ADDR_IN_SLOT_B(addr)

/** @} */

#endif /* IMAGE_CONFIG_H */
