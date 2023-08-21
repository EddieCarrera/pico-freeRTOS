# Note: This CMake enviroment was created from "pico-examples"
# and throwing away everything but the "pico_w" directory
# Thing were also changed in all the CMakeLists.txt to ensure 
# compatibility 

# Learning RTOS with a multicore RP2040 Pico W target
cmake_minimum_required(VERSION 3.12)

# Needed for pico_w_examples
set(WIFI_SSID "Chronos")
set(WIFI_PASSWORD "helloWorld1324$")

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Must set board type, else "regular" pico board is assumed
set(PICO_BOARD pico_w)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

include(example_auto_set_url.cmake)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

# Hardware-specific examples in subdirectories:
add_subdirectory(pico_w_examples)
add_subdirectory("Mastering the FreeRTOS Kernel")
