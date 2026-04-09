#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/memorymap.h>
#include "bl-flash.h"
#include "libopencm3/stm32/common/flash_common_all.h"


#define PAGE_SIZE (0x0400)
#define MAIN_APPLICATION_START (FLASH_BASE + (PAGE_SIZE * 32U))
#define FLASH_END (FLASH_BASE + (PAGE_SIZE) * 64U)
#define WORD_SIZE (4U) // in bytes.

void bl_flash_erase_main_application(void) {
    flash_unlock(); 
    for (uint32_t page_address = MAIN_APPLICATION_START; page_address < FLASH_END; page_address+=PAGE_SIZE) {
        flash_erase_page(page_address);
    }
    flash_lock(); 
}

void bl_flash_write(const uint32_t address, const uint32_t* data, const uint32_t length) {
    flash_unlock();
    for (uint32_t i = 0; i < length; i ++) {
        flash_program_word(address + (i * WORD_SIZE), data[i]);
    }
    flash_lock();
}
