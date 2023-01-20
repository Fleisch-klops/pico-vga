cmake_minimum_required(VERSION 3.12)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(vga_interface C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(${CMAKE_PROJECT_NAME}
    main.c
)

pico_generate_pio_header(${CMAKE_PROJECT_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/vga.pio
)

pico_enable_stdio_usb(${CMAKE_PROJECT_NAME} 1)
pico_enable_stdio_uart(${CMAKE_PROJECT_NAME} 0)

pico_add_extra_outputs(${CMAKE_PROJECT_NAME})

target_link_libraries(${CMAKE_PROJECT_NAME}
    pico_stdlib
    hardware_pio
    hardware_flash
    hardware_dma
)

#cmake -G "MinGW Makefiles" ..