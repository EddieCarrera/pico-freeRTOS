#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

void led_task()
{    
    while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(250);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(250);
    }
}

int main()
{
    stdio_init_all();

    // LED is driven by the cyw43 chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }
    xTaskCreate((TaskFunction_t)led_task, "LED_Task", 256, NULL, 1, NULL);
    vTaskStartScheduler();

    while(1){};
}