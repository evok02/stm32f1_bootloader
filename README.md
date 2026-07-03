# Bare Metal Firmware Update Bootloader — STM32F103C8T6

A hands-on bare-metal firmware project for the STM32F103C8T6 (Blue Pill) featuring a UART-based bootloader for over-the-wire firmware updates.

## Directory Structure

```
bare_metal_tutor/
├── app/                    # Main application firmware
│   ├── inc/                # App headers
│   │   ├── common.h
│   │   └── timer.h
│   ├── src/
│   │   ├── firmware.c      # Main app: LED blink + UART echo
│   │   ├── timer.c         # PWM timer setup (TIM2)
│   │   └── bootloader.S    # Embeds bootloader.bin via .incbin
│   ├── linkerscript.ld     # 64KB flash / 20KB RAM
│   └── Makefile
├── bootloader/             # UART-based firmware update bootloader
│   ├── inc/
│   │   ├── common.h
│   │   ├── bl-flash.h      # Flash erase/write abstraction
│   │   └── comms.h         # Packet protocol definitions & API
│   ├── src/
│   │   ├── bootloader.c    # State machine: sync → update → jump
│   │   ├── bl-flash.c      # STM32 flash page erase + word program
│   │   └── comms.c         # Packet framing, CRC, retransmit
│   ├── linkerscript.ld     # 32KB flash / 20KB RAM
│   ├── pad-bootloader.py   # Pads binary to exactly 32KB
│   └── Makefile
├── shared/                 # Shared modules used by app & bootloader
│   ├── inc/core/
│   │   ├── system.h        # SysTick-based 1ms tick & delay
│   │   ├── uart.h          # USART1 with IRQ + ring buffer
│   │   ├── crc8.h          # CRC-8 (polynomial 0x07)
│   │   ├── ring-buffer.h   # Lock-free ring buffer
│   │   └── simple-timer.h  # Millisecond timeout helper
│   └── src/core/
│       ├── system.c
│       ├── uart.c
│       ├── crc8.c
│       ├── ring-buffer.c
│       └── simple-timer.c
├── fw-updater/             # TypeScript host tool (runs on PC)
│   ├── index.ts            # Serial protocol client
│   ├── package.json
│   └── tsconfig.json
├── scripts/
│   ├── debug.sh            # GDB + OpenOCD for app
│   └── debug-bootloader.sh # GDB + OpenOCD for bootloader
└── libopencm3/             # Git submodule — ARM Cortex-M HAL
```

## Components

### Bootloader (`bootloader/`)

Occupies the first 32KB of flash (`0x08000000`). Its state machine implements a firmware update protocol over UART (115200 baud):

1. **SYNC** — waits for a 4-byte sync sequence (`0xC4 0x55 0x7E 0x10`)
2. **FW_UPDATE_REQ** — receives update request, acknowledges
3. **DEVICE_ID** — requests device ID from host, validates response (`0x42`)
4. **FW_LENGTH** — requests firmware length, validates against max (32KB)
5. **ERASE** — erases application flash pages (32KB–64KB)
6. **RECEIVE_FW** — accepts 16-byte data packets, programs flash word-by-word
7. **DONE** — sends success, tears down peripherals, jumps to application

### Application (`app/`)

The main firmware starts at `0x08008000`. It:
- Redirects the vector table to the application offset via `SCB_VTOR`
- Blinks an LED (PC13) every second
- Echoes received UART bytes with `+ 1` offset
- Embeds `bootloader.bin` via `bootloader.S` (`.incbin`), so a single flash write programs both images

### Shared Library (`shared/`)

Provides the common HAL layer:

| Module | Purpose |
|---|---|
| `system` | RCC (72MHz HSE PLL), SysTick 1ms tick, `delay()` |
| `uart` | USART1 interrupt-driven with ring buffer |
| `crc8` | CRC-8 with polynomial `0x07` |
| `ring-buffer` | Lock-free single-consumer ring buffer |
| `simple-timer` | Millisecond timeout with optional auto-reset |

### fw-updater (`fw-updater/`)

A Node.js/TypeScript host utility that communicates with the bootloader over a serial port (`/dev/ttyUSB0`, 115200 baud). It reads `app/firmware.bin` (skipping the first 32KB bootloader region) and walks the bootloader state machine to flash the application.

```bash
cd fw-updater && npm run build && npm start
```

## Protocol

Fixed 18-byte packet: `[1B len][16B data][1B CRC-8]`

| Direction | Packet | Code |
|---|---|---|
| Host → Target | Sync sequence | `0xC4 0x55 0x7E 0x10` |
| Target → Host | Sync observed | `0x20` |
| Host → Target | FW update request | `0x31` |
| Target → Host | FW update response | `0x37` |
| Target → Host | Device ID request | `0x3C` |
| Host → Target | Device ID response | `0x3F` + `0x42` |
| Target → Host | FW length request | `0x45` |
| Host → Target | FW length response | `0x45` + 4B LE length |
| Target → Host | Ready for data | `0x48` |
| Target → Host | Update successful | `0x54` |
| Target → Host | NACK / abort | `0x59` |
| Either | ACK | `0x15` |
| Either | Retransmit (RETX) | `0x19` |

## Memory Map

| Region | Start | Size | Content |
|---|---|---|---|
| Flash | `0x08000000` | 32KB | Bootloader |
| Flash | `0x08008000` | 32KB | Application |
| RAM | `0x20000000` | 20KB | Stack, data, BSS, `.noinit` |

## Build & Flash

```bash
# Build bootloader
cd bootloader && make

# Build application (embeds bootloader.bin)
cd app && make

# Flash bootloader + app together
cd app && make flash

# Or flash bootloader standalone (only if modifying the bootloader itself)
cd bootloader && make flash
```

## Debugging

Requires OpenOCD running on `localhost:3333` (e.g. ST-Link v2).

```bash
# Debug application
./scripts/debug.sh

# Debug bootloader
./scripts/debug-bootloader.sh
```

## Dependencies

- `arm-none-eabi` toolchain (GCC, binutils, newlib-nano)
- `libopencm3` (included as git submodule)
- `st-flash` (for flashing via ST-Link)
- OpenOCD (for debugging)
- Node.js 18+ and `npm` (for `fw-updater`)
