#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *********************************
 * 1) When a task goes into the block state, the scheduler will select the next 
 * highest priority task to run even BEFORE the next tick.
 *   
 * 2) vTaskDelayUntil() is used by periodic tasks to ensure a constant execution 
 * frequency. vTaskDelay() will cause a task to block for the specified number of 
 * ticks from the time vTaskDelay() is called. It is therefore difficult to use 
 * vTaskDelay() by itself to generate a fixed execution frequency as the time between 
 * a task unblocking following a call to vTaskDelay() and that task next calling 
 * vTaskDelay() may not be fixed [the task may take a different path through the 
 * code between calls, or may get interrupted or preempted a different number of 
 * times each time it executes]. 
 *******************************************************************************/

#define TASK1_PRIORITY          1
#define TASK2_PRIORITY          1
#define PERIODIC_TASK_PRIORITY  2

// TaskHandle_t task1 = NULL;
// TaskHandle_t task2 = NULL;
// TaskHandle_t periodicTask = NULL;
const char *pcTextforTask1 = "Task1 is running!\r\n";
const char *pcTextforTask2 = "Task2 is running!\r\n";
const char *pcTextforPerodicTask = "periodicTask is running!\r\n";

void vTask1(void *pvParameters)
{
    TickType_t prev_wakeTime;
    char *textParam = (char*)pvParameters;

    /* The xLastWakeTime variable needs to be initialized with the current tick
    count.  Note that this is the only time the variable is written to explicitly.
    After this xLastWakeTime is automatically updated within vTaskDelayUntil(). */
    prev_wakeTime = xTaskGetTickCount();
    while(1)
    {
        printf("%s", textParam);
        // vTaskDelay(delay250ms); // Don't use
        // vTaskDelayUntil( &prev_wakeTime, pdMS_TO_TICKS(250) );
    }
}

void vTask2(void *pvParameters)
{
    TickType_t prev_wakeTime;
    char *textParam = (char*)pvParameters;
    prev_wakeTime = xTaskGetTickCount();
    while(1)
    {
        printf("%s", textParam);
        // vTaskDelayUntil( &prev_wakeTime, pdMS_TO_TICKS(250) );
    }
}

void vperiodicTask(void *pvParameters)
{
    TickType_t prev_wakeTime;
    char *textParam = (char*)pvParameters;
    prev_wakeTime = xTaskGetTickCount();
    while(1)
    {
        printf("%s", textParam);
        vTaskDelayUntil( &prev_wakeTime, pdMS_TO_TICKS(5) );
    }
}

int main()
{
    stdio_init_all();

    xTaskCreate(vTask1, "TASK 1", configMINIMAL_STACK_SIZE,
                (void*)pcTextforTask1, TASK1_PRIORITY, NULL);
    xTaskCreate(vTask2, "TASK 2", configMINIMAL_STACK_SIZE,
                (void*)pcTextforTask2, TASK2_PRIORITY, NULL);
    xTaskCreate(vperiodicTask, "periodicTask", configMINIMAL_STACK_SIZE,
                (void*)pcTextforPerodicTask, PERIODIC_TASK_PRIORITY, NULL);
       
    vTaskStartScheduler();
 /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks.  If main() does reach here then it is likely that
    there was insufficient heap memory available for the idle task to be created */
    while(1);
}