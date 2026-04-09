#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/common/gpio_common_all.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/scb.h>

#include "core/system.h"
#include "timer.h"
#include "core/uart.h"

#define BOOTLOADER_SIZE (0x8000U)

#define LED_PORT GPIOC
#define LED_PIN  GPIO13

#define UART_PORT (GPIOA)
#define TX_PIN (GPIO9)
#define RX_PIN (GPIO10)

static void vector_setup(void) {
    SCB_VTOR = BOOTLOADER_SIZE;
}

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_50_MHZ,
              GPIO_CNF_OUTPUT_OPENDRAIN, LED_PIN);

    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_set_mode(UART_PORT, GPIO_MODE_OUTPUT_50_MHZ,
              GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, TX_PIN | RX_PIN);
}

int main(void) {
    vector_setup();
    system_setup();
    gpio_setup();
    timer_setup();
    uart_setup();

    uint64_t start_time = system_get_ticks();

    for (;;) {
        if (system_get_ticks() - start_time >= 1000) {
            gpio_toggle(LED_PORT, LED_PIN);
            start_time = system_get_ticks();
        }

        if (uart_data_available()) {
            uint8_t data = uart_read_byte();
            uart_write_byte(data + 1);
        }

    }

    return 0;
}
