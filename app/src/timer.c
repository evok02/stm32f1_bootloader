#include "timer.h"
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>

// system_freq = 72e3
// freq = system_freq / (prescaler * arr) 

#define PRESCALER (72)
#define ARR_VALUE (1000)

void timer_setup(void) {
    rcc_periph_clock_enable(RCC_TIM2);

    // Hight level timer configuration.
    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    // Set up PWM mode.
    timer_set_oc_mode(TIM2, TIM_OC2, TIM_OCM_PWM1);

    // Setup frequency and resolution.
    timer_set_prescaler(TIM2, PRESCALER - 1);
    timer_set_period(TIM2, ARR_VALUE - 1);

    timer_generate_event(TIM2, TIM_EGR_UG);

    // Enable counter.
    timer_enable_counter(TIM2);
    timer_enable_oc_output(TIM2, TIM_OC2);

}

void timer_pwm_set_duty_cycle(float duty_cycle) {
    // duty cycle = (ccr / arr) * 100 
    // ccr = arr * (duty cycle / 100)

    const float raw_value = (float)ARR_VALUE * (duty_cycle / 100.0f);
    timer_set_oc_value(TIM2, TIM_OC2, (uint32_t)raw_value);
}
