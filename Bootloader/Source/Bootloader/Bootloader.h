#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdbool.h>
#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        Slot_A = 0,
        Slot_B
    } Bootloader_Slot_t;

    bool Bootloader_Verify_Slot(Bootloader_Slot_t slot);
    void Bootloader_Jump_To_App(Bootloader_Slot_t slot);
    void Bootloader_Default();

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_H */
