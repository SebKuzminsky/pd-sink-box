cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include("$ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake")

include("$ENV{PICO_EXAMPLES_PATH}/pico_extras_import_optional.cmake")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.5.1")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.1 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

add_compile_options(
    -Wall
)

project("pd-sink-box_${PICO_BOARD}" C CXX ASM)

pico_sdk_init()


add_subdirectory("submodules/rp2040-rotary-encoder/driver")
add_subdirectory("submodules/rp2040-husb238/driver")

add_subdirectory("submodules/hagl")
add_subdirectory("submodules/hagl_hal")

set(PROGRAM_NAME pd-sink-box)

add_executable(
    ${PROGRAM_NAME}
    main.cpp
    hmi.cpp
    hagl_char_scaled.c
)


add_custom_target(
    regenerate-version-info
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/make-version-info" "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_dependencies(
    ${PROGRAM_NAME}
    regenerate-version-info
)


#
# Waveshare 1.14inch LCD Module (240x135 ST7789VW)
# https://www.waveshare.com/wiki/1.14inch_LCD_Module
#
target_compile_definitions(
    ${PROGRAM_NAME} PRIVATE
    MIPI_DISPLAY_PIN_CS=12
    MIPI_DISPLAY_PIN_DC=11
    MIPI_DISPLAY_PIN_RST=10
    MIPI_DISPLAY_PIN_BL=9
    MIPI_DISPLAY_PIN_CLK=14
    MIPI_DISPLAY_PIN_MOSI=15
    MIPI_DISPLAY_PIN_MISO=-1
    MIPI_DISPLAY_PIN_POWER=-1
    MIPI_DISPLAY_PIN_VSYNC=-1

    MIPI_DISPLAY_SPI_PORT=spi1
    MIPI_DISPLAY_SPI_CLOCK_SPEED_HZ=62500000

    MIPI_DISPLAY_PIXEL_FORMAT=MIPI_DCS_PIXEL_FORMAT_16BIT
    MIPI_DISPLAY_ADDRESS_MODE=MIPI_DCS_ADDRESS_MODE_RGB
    # MIPI_DISPLAY_ADDRESS_MODE=MIPI_DCS_ADDRESS_MODE_RGB|MIPI_DCS_ADDRESS_MODE_SWAP_XY|MIPI_DCS_ADDRESS_MODE_MIRROR_X
    MIPI_DISPLAY_WIDTH=135
    MIPI_DISPLAY_HEIGHT=240
    MIPI_DISPLAY_OFFSET_X=52
    MIPI_DISPLAY_OFFSET_Y=40
    MIPI_DISPLAY_INVERT=1
)

target_compile_definitions(
    ${PROGRAM_NAME} PRIVATE
    #  HAGL_HAL_USE_SINGLE_BUFFER
    HAGL_HAL_USE_DOUBLE_BUFFER
    #  HAGL_HAL_USE_TRIPLE_BUFFER
    #  HAGL_HAL_USE_DMA
    #  HAGL_HAL_DEBUG
    HAGL_HAL_PIXEL_SIZE=1
)

target_include_directories(
    ${PROGRAM_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(
    ${PROGRAM_NAME}
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_pwm
    hardware_flash
    hardware_sync
    hagl
    hagl_hal
    rp2040_rotary_encoder
    rp2040_husb238
)

if ("${PICO_BOARD}" STREQUAL "pico_w")
    target_link_libraries(
        ${PROGRAM_NAME}
        pico_cyw43_arch_none
    )
endif()


pico_enable_stdio_usb(${PROGRAM_NAME} TRUE)
pico_enable_stdio_uart(${PROGRAM_NAME} TRUE)

pico_add_extra_outputs(${PROGRAM_NAME})
