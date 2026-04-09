#! /usr/bin/sh

gdb -ex 'target extended-remote localhost:3333' \
    -ex 'load' \
    -ex 'break *0x08000000' \
    -ex 'c' \
    -ex 'tui enable' \
    ../app/firmware.elf
