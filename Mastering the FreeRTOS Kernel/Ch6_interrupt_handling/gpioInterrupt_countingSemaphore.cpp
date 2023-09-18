#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <semphr.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *****************************
 * 1) 
*****************************************************************************/

#define GPIO_PIN 9

SemaphoreHandle_t binarySemaphore;

void gpio_callback(uint gpio, uint32_t events)
{
    /* The xHigherPriorityTaskWoken parameter must be initialized to pdFALSE as
    it will get set to pdTRUE inside the interrupt safe API function if a
    context switch is required. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    printf("ISR triggered\r\n");

    // event is an enum gpio_irq_level
    if (gpio == GPIO_PIN)
    {

        /* 'Give' the semaphore to unblock the task, passing in the address of
        xHigherPriorityTaskWoken as the interrupt safe API function's
        pxHigherPriorityTaskWoken parameter. 
        
        See NOTE 3
        */
        xSemaphoreGiveFromISR(binarySemaphore, &xHigherPriorityTaskWoken);

        /* Pass the xHigherPriorityTaskWoken value into portYIELD_FROM_ISR().  If
        xHigherPriorityTaskWoken was set to pdTRUE inside xSemaphoreGiveFromISR()
        then calling portYIELD_FROM_ISR() will request a context switch.  If
        xHigherPriorityTaskWoken is still pdFALSE then calling
        portYIELD_FROM_ISR() will have no effect. */
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void gpio_triggered_task(void *TaskParam)
{
    while(1)
    {
        /* Use the semaphore to wait for the event.  The semaphore was created
        before the scheduler was started, so before this task ran for the first
        time. The task blocks indefinitely, meaning this function call will only
        return once the semaphore has been successfully obtained - so there is
        no need to check the value returned by xSemaphoreTake(). */
        xSemaphoreTake(binarySemaphore, portMAX_DELAY);

        /* To get here the event must have occurred.  Process the event (in this
        Case, just print out a message). */
        printf( "Handler task - Processing event.\r\n" );
    }
}

int main()
{
    stdio_init_all();
    while(!stdio_usb_connected()){tight_loop_contents();}

    sleep_ms(1000);
    printf("ISR Semaphore example\r\n");
    gpio_pull_up(GPIO_PIN);
    gpio_set_irq_enabled_with_callback(GPIO_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    /* Before a semaphore is used it must be explicitly created.  In this example
    a binary semaphore is created. */
    binarySemaphore = xSemaphoreCreateBinary();
    /* Check the semaphore was created successfully. */
    if (binarySemaphore != NULL)
    {
        /* Create the 'handler' task, which is the task to which interrupt
        processing is deferred.  This is the task that will be synchronized with
        the interrupt.  The handler task is created with a high priority to ensure
        it runs immediately after the interrupt exits.  In this case a priority of
        3 is chosen. */
        xTaskCreate(gpio_triggered_task, "Handler", configMINIMAL_STACK_SIZE, NULL, 3, NULL );

        /* Start the scheduler so the created tasks start executing. */
        vTaskStartScheduler();
    }
    /* As normal, the following line should never be reached. */
    for( ;; );
}