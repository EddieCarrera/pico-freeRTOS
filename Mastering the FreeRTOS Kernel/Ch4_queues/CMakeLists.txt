set(OUTPUT_NAME preemtive_queuing queuing_pointers_string)
set(SOURCES preemtive_queuing.cpp queuing_pointers_string.cpp)

foreach(OUTPUT SOURCE IN ZIP_LISTS OUTPUT_NAME SOURCES)
    add_executable(${OUTPUT} ${SOURCE})

    target_include_directories(${OUTPUT} PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/.. # For FreeRTOSConfig.h
            )

    target_link_libraries(${OUTPUT}
            pico_stdlib
            FreeRTOS-Kernel-Heap4   # FreeRTOS kernel and dynamic heap
            pico_cyw43_arch_none    # We need Wifi to access the GPIO, but we don't need anything else
            )

    # enable usb output, disable uart output
    pico_enable_stdio_usb(${OUTPUT} 1)
    pico_enable_stdio_uart(${OUTPUT} 0)

    pico_add_extra_outputs(${OUTPUT})
endforeach()
