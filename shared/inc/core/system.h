#ifndef INC_SYSTEM_H
#define INC_SYSTEM_H

#include "common.h"

#define CPU_FREQ_HZ (72e6)
#define TIMER_FREQ_MS  (1e3)


void system_setup(void);
uint64_t system_get_ticks(void);
void system_delay(uint64_t milleseconds);
void system_teardown(void);

#endif
