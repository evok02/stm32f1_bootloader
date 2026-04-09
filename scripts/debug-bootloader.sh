#! /usr/bin/sh


gdb -ex 'target extended-remote localhost:3333' \
    -ex 'load' \
    -ex 'break main' \
    -ex 'c' \
    -ex 'tui enable' \
    ../bootloader/bootloader.elf
