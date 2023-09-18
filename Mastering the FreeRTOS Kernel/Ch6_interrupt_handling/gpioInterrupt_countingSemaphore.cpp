#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <semphr.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *********************************
 * 1) Just as binary semaphores can be thought of as queues that have a 
 * length of one, counting semaphores can be thought of as queues that have 
 * a length of more than one. Tasks are not interested in the data that is 
 * stored in the queue—just the number of items in the queue. Counting semaphores 
 * are typically used for two things: counting events and resource management.
 * 
 * Counting events: In this scenario, an event handler will ‘give’ a semaphore 
 * each time an event occurs—causing the semaphore’s count value to be incremented 
 * on each ‘give’. A task will ‘take’ a semaphore each time it processes an 
 * event—causing the semaphore’s count value to be decremented on each ‘take’. 
 * The count value is the difference between the number of events that have 
 * occurred and the number that have been processed. 
 * 
 * Resource management: the count value indicates the number of resources available. 
 * To obtain control of a resource, a task must first obtain a semaphore—decrementing 
 * the semaphore’s count value. When the count value reaches zero, there are no free
 * resources. When a task finishes with the resource, it ‘gives’ the semaphore 
 * back—incrementing the semaphore’s count value. 
 * 
 * NOTE: It is more efficient to count events using a direct to task notification 
 * than it is using a counting semaphore. Direct to task notifications are not 
 * covered until Chapter 9
 *******************************************************************************/

#define GPIO_PIN 9

SemaphoreHandle_t countingSemaphore;

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

        /* 'Give' the semaphore multiple times. The first will unblock the deferred
        interrupt handling task, the following 'gives' are to demonstrate that the
        semaphore latches the events to allow the task to which interrupts are deferred
        to process them in turn, without events getting lost. This simulates multiple
        interrupts being received by the processor, even though in this case the events
        are simulated within a single interrupt occurrence. */
        xSemaphoreGiveFromISR(countingSemaphore, &xHigherPriorityTaskWoken);
        asm volatile("nop \n nop \n nop");
        xSemaphoreGiveFromISR(countingSemaphore, &xHigherPriorityTaskWoken);
        asm volatile("nop \n nop \n nop");
        xSemaphoreGiveFromISR(countingSemaphore, &xHigherPriorityTaskWoken);

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
        xSemaphoreTake(countingSemaphore, portMAX_DELAY);

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

    /* Before a semaphore is used it must be explicitly created. In this example a
    counting semaphore is created. The semaphore is created to have a maximum count
    value of 10, and an initial count value of 0. */
    countingSemaphore = xSemaphoreCreateCounting(10, 0);
    /* Check the semaphore was created successfully. */
    if (countingSemaphore != NULL)
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