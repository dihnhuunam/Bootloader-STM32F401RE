#ifndef IMAGE_H
#define IMAGE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define SLOT_A_BASE_ADDR 0x08010000UL // Slot A start address
#define SLOT_A_SIZE 0x00030000UL      // 192 Kbytes
#define SLOT_A_END_ADDR (SLOT_A_BASE_ADDR + SLOT_A_SIZE)
#define APP_MAGIC_NUMBER 0xA5A5A5A5 // Magic number to verify application
#define VTABLE_OFFSET 0x200U        /* 512 bytes */
#define APP_VECTOR_TABLE_ADDR (SLOT_A_BASE_ADDR + VTABLE_OFFSET)
#define SRAM_START_ADDR 0x20000000UL
#define SRAM_END_ADDR 0x20018000UL

    typedef struct __attribute__((packed))
    {
        uint32_t magic;
        uint32_t version;
        uint32_t crc32;
        uint32_t reserved;
    } ImageHeader_t;

    typedef struct __attribute__((packed))
    {
        uint32_t stack_pointer; // SP address
        uint32_t reset_handler; // Reset Handler address
    } VectorTable_t;

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_H */
