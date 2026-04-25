#ifndef IMAGE_H
#define IMAGE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define SLOT_A_BASE_ADDR 0x08010000UL // Slot A start address
#define APP_MAGIC_NUMBER 0xA5A5A5A5 // Magic number to verify application

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
