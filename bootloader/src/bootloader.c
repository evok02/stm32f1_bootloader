#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/cm3/vector.h>

#include "core/uart.h"
#include "core/system.h"
#include "core/simple-timer.h"
#include "comms.h"
#include "bl-flash.h"

#define BOOTLOADER_SIZE (0x8000U)
#define MAIN_START_ADDR (FLASH_BASE + BOOTLOADER_SIZE)

#define UART_PORT (GPIOA)
#define TX_PIN (GPIO9)
#define RX_PIN (GPIO10)

#define DEVICE_ID (0x42)

#define SYN_SEQ_0 (0xC4)
#define SYN_SEQ_1 (0x55)
#define SYN_SEQ_2 (0x7E)
#define SYN_SEQ_3 (0x10)

#define MAX_FIRMWARE_LENGTH ((1024U * 64U) - BOOTLOADER_SIZE)

#define DEFAULT_TIMEOUT (5e3)

typedef enum bl_sate_t {
    BL_STATE_SYNC,
    BL_STATE_WAIT_FOR_UPDATE_REQ,
    BL_STATE_DEVICE_ID_REQ,
    BL_STATE_DEVICE_ID_RES,
    BL_STATE_FW_LENGTH_REQ,
    BL_STATE_FW_LENGTH_RES,
    BL_STATE_ERASE_APPLICATION,
    BL_STATE_RECEIVE_FW,
    BL_STATE_DONE,
} bl_state_t;

// Default values.
static bl_state_t state = BL_STATE_SYNC;
static uint32_t fw_length = 0;
static uint32_t bytes_written = 0;
static uint8_t syn_seq[4] = {0};
static simple_timer_t temp_timer;
static comms_packet_t temp_packet;

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_set_mode(UART_PORT, GPIO_MODE_OUTPUT_50_MHZ,
              GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, TX_PIN | RX_PIN);
}

static void gpio_teardown(void) {
    gpio_set_mode(UART_PORT, GPIO_MODE_INPUT,
              GPIO_CNF_INPUT_ANALOG, TX_PIN | RX_PIN);
    rcc_periph_clock_disable(RCC_GPIOA);
}

static void jump_to_main(void) {
    vector_table_t* main_vector_table = (vector_table_t*)MAIN_START_ADDR;
    main_vector_table->reset();
}

static void bootloader_exit(void) {
    comms_create_single_byte_packet(&temp_packet, BL_PACKET_NACK_DATA0);
    comms_write(&temp_packet);
    state = BL_STATE_DONE;
}

static void check_for_timeout(void) {
    if (simple_timer_has_elapsed(&temp_timer)) {
        bootloader_exit();
    }
}

static bool is_device_id_packet(const comms_packet_t* packet) {
    if (packet->length != 2) {
        return false;
    }
    if (packet->data[0] != BL_PACKET_DEVICE_ID_RES_DATA0) {
        return false;
    }
    for (uint8_t i = 2; i < PACKET_DATA_LENGTH; i++) {
        if (packet->data[i] != 0xff) {
            return false;
        }
    }

    return true;
}

static bool is_fw_length_packet(const comms_packet_t* packet) {
    if (packet->length != 5) {
        return false;
    }
    if (packet->data[0] != BL_PACKET_FW_LENGTH_RES_DATA0) {
        return false;
    }
    for (uint8_t i = 5; i < PACKET_DATA_LENGTH; i++) {
        if (packet->data[i] != 0xff) {
            return false;
        }
    }

    return true;
}

int main(void) {
    system_setup();
    gpio_setup();
    uart_setup();
    comms_setup();

    simple_timer_setup(&temp_timer, DEFAULT_TIMEOUT, false);

    while (state != BL_STATE_DONE) {
        if (state == BL_STATE_SYNC) {
            if (uart_data_available()) {
                syn_seq[0] = syn_seq[1];
                syn_seq[1] = syn_seq[2];
                syn_seq[2] = syn_seq[3];
                syn_seq[3] = uart_read_byte();

                bool is_match = syn_seq[0] == SYN_SEQ_0;
                is_match = is_match && ( syn_seq[1] == SYN_SEQ_1 );
                is_match = is_match && ( syn_seq[2] == SYN_SEQ_2 );
                is_match = is_match && ( syn_seq[3] == SYN_SEQ_3 );

                if (is_match) {
                    comms_create_single_byte_packet(&temp_packet, BL_PACKET_SYNC_OBSERVED_DATA0);
                    comms_write(&temp_packet);
                    simple_timer_reset(&temp_timer);
                    state = BL_STATE_WAIT_FOR_UPDATE_REQ;
                } else {
                    check_for_timeout();
                }
            } else {
                check_for_timeout();
            } 
            continue;
        }

        comms_update();

        switch (state) {
            case BL_STATE_WAIT_FOR_UPDATE_REQ: {
                if (comms_packets_available()) {
                    comms_read(&temp_packet);    
                    if (comms_is_single_byte_packet(&temp_packet, BL_PACKET_FW_UPDATE_REQ_DATA0)) {
                        simple_timer_reset(&temp_timer);
                        comms_create_single_byte_packet(&temp_packet, BL_PACKET_FW_UPDATE_RES_DATA0);
                        comms_write(&temp_packet);
                        state = BL_STATE_DEVICE_ID_REQ;
                    } else {
                       bootloader_exit(); 
                    }
                } else {
                    check_for_timeout();
                }
            } break;
            case BL_STATE_DEVICE_ID_REQ: {
                simple_timer_reset(&temp_timer);
                comms_create_single_byte_packet(&temp_packet, BL_PACKET_DEVICE_ID_REQ_DATA0);
                comms_write(&temp_packet);
                state = BL_STATE_DEVICE_ID_RES;
            } break;
            case BL_STATE_DEVICE_ID_RES: {
                if (comms_packets_available()) {
                    comms_read(&temp_packet);    
                    if (is_device_id_packet(&temp_packet) && (temp_packet.data[1] == DEVICE_ID)) {
                        simple_timer_reset(&temp_timer);
                        state = BL_STATE_DEVICE_ID_REQ;
                    } else {
                       bootloader_exit(); 
                    }
                } else {
                    check_for_timeout();
                }
            } break;
            case BL_STATE_FW_LENGTH_REQ: {
                simple_timer_reset(&temp_timer);
                comms_create_single_byte_packet(&temp_packet, BL_PACKET_FW_LENGTH_REQ_DATA0);
                state = BL_STATE_FW_LENGTH_RES;
            } break;
            case BL_STATE_FW_LENGTH_RES: {
                if (comms_packets_available()) {
                    comms_read(&temp_packet); 

                    fw_length = (
                        (temp_packet.data[1]) |
                        (temp_packet.data[2]) << 8 |
                        (temp_packet.data[3]) << 16 |
                        (temp_packet.data[4]) << 24 
                    );
                    if (is_fw_length_packet(&temp_packet) && (fw_length <= MAX_FIRMWARE_LENGTH)) {
                        simple_timer_reset(&temp_timer);
                        state = BL_STATE_DEVICE_ID_REQ;
                    } else {
                       bootloader_exit(); 
                    }
                } else {
                    check_for_timeout();
                }
            } break;
            case BL_STATE_ERASE_APPLICATION: {
                bl_flash_erase_main_application();
                simple_timer_reset(&temp_timer);
                state = BL_STATE_RECEIVE_FW;
            } break;
            case BL_STATE_RECEIVE_FW: {
                if (comms_packets_available()) {
                    comms_read(&temp_packet);
                    const uint8_t packet_length = ((temp_packet.length & 0x0f) + 1);
                    bl_flash_write(MAIN_START_ADDR + bytes_written, (uint32_t*)temp_packet.data, packet_length);
                    bytes_written += packet_length;
                    simple_timer_reset(&temp_timer);

                    if (bytes_written >= fw_length) {
                        comms_create_single_byte_packet(&temp_packet, BL_PACKET_UPDATE_SUCCESSFUL_DATA0);

                        state = BL_STATE_DONE;
                    }
                } else {
                    check_for_timeout();
                }
            } break;
            default: {
                state = BL_STATE_SYNC;
            }
            
        }
    }

    // : ((((
    system_delay(150);

    uart_teardown();
    gpio_teardown();
    system_teardown();

    jump_to_main();

    return 0;
}
