#ifndef INC_BL_FLASH
#define INC_BL_FLASH

#include "common.h"

void bl_flash_erase_main_application(void);
void bl_flash_write(const uint32_t address, const uint32_t* data, const uint32_t length);

#endif
