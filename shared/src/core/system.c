#include "core/system.h"
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/rcc.h>

static volatile uint64_t ticks = 0;

static void rcc_setup(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

void sys_tick_handler(void) {
    ticks++;    
}

static void systick_setup(void) {
    systick_set_frequency(TIMER_FREQ_MS, CPU_FREQ_HZ);
    systick_counter_enable();
    systick_interrupt_enable();
}

uint64_t system_get_ticks(void) {
    return ticks;
}

void system_setup(void) {
    rcc_setup();
    systick_setup();    
}

void system_teardown(void) {
    systick_interrupt_disable();
    systick_counter_disable();
    systick_clear();
}

void system_delay(uint64_t milleseconds) {
    uint64_t end_time = system_get_ticks() + milleseconds;
    while (system_get_ticks() < end_time) {
        // Spin
    }
}

