#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdbool.h>
#ifdef __cplusplus
extern "C"
{
#endif

    bool Bootloader_Verify_Slot();
    void Bootloader_Jump_To_App();
    void Bootloader_Default();

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_H */
