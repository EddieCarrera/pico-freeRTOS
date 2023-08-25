set(OUTPUT_NAME timers)
add_executable(${OUTPUT_NAME} timers.cpp)

target_include_directories(${OUTPUT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/.. # For FreeRTOSConfig.h
        )

target_link_libraries(${OUTPUT_NAME}
        pico_stdlib
        FreeRTOS-Kernel-Heap4   # FreeRTOS kernel and dynamic heap
        pico_cyw43_arch_none    # We need Wifi to access the GPIO, but we don't need anything else
        )

# enable usb output, disable uart output
pico_enable_stdio_usb(${OUTPUT_NAME} 1)
pico_enable_stdio_uart(${OUTPUT_NAME} 0)

pico_add_extra_outputs(${OUTPUT_NAME})