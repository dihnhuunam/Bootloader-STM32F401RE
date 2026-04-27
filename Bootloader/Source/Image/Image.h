#ifndef IMAGE_H
#define IMAGE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    typedef struct __attribute__((packed))
    {
        uint32_t magic;
        uint32_t version;
        uint32_t crc32;
        uint32_t image_size;
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
