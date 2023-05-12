#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#define TASK1_PRIORITY 0
#define TASK2_PRIORITY 0

const char msg[] = "Fourier did nothing wrong/r/n";

TaskHandle_t task1 = NULL;
TaskHandle_t task2 = NULL;

void vTask1(void *pvParameters)
{
    while(1)
    {
        printf("%s", pcTaskGetName);
        sleep_ms(100);
    }
}

void vTask2(void *pvParameters)
{
    while(1)
    {
        printf("%s", pcTaskGetName);
        sleep_ms(100);
    }
}

int main()
{
    stdio_init_all();

    bool error = false;
    error |= xTaskCreate(
                vTask1, 
                "TASK 1", 
                configMINIMAL_STACK_SIZE,
                NULL,
                TASK1_PRIORITY,
                NULL
                );

    error |= xTaskCreate(vTask2, "TASK 2", configMINIMAL_STACK_SIZE,
                        NULL, TASK2_PRIORITY, NULL);

    if (error != true)
    {
        printf("Task creation failed");
    }
    
    vTaskStartScheduler();

 /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks.  If main() does reach here then it is likely that
    there was insufficient heap memory available for the idle task to be created */
    while(1);
    return 0;
}