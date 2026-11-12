# pinewood-time firmware for the Raspberry Pi Pico (RP2040). Build:
#   cmake -B build -DPICO_SDK_PATH=/path/to/pico-sdk -DPICO_BOARD=pico
#   cmake --build build
# The result is build/pinewood_time.uf2: hold BOOTSEL, plug the Pico in, and copy it to the drive that appears.
cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pinewood_time C CXX ASM)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

add_executable(pinewood_time
    src/main.c
    src/timer_core.c
    src/display.c
    src/settings.c
)

target_compile_options(pinewood_time PRIVATE -Wall -Wextra -Werror)

# How it shows up on the computer
target_compile_definitions(pinewood_time PRIVATE
    USBD_MANUFACTURER="no5tyle"
    USBD_PRODUCT="pinewood-time race timer"
)

target_link_libraries(pinewood_time
    pico_stdlib
    pico_flash
    hardware_i2c
    hardware_flash
    hardware_gpio
)

# Talk over USB (it shows up as a serial port), not the UART pins
pico_enable_stdio_usb(pinewood_time 1)
pico_enable_stdio_uart(pinewood_time 0)

pico_set_program_name(pinewood_time "pinewood-time")
pico_set_program_version(pinewood_time "1.0.0")
pico_set_program_url(pinewood_time "https://github.com/no5tyle/pinewood-time")

pico_add_extra_outputs(pinewood_time)
