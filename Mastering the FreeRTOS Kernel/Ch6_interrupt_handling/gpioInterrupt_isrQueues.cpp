#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <semphr.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *********************************
 * 1) Binary and counting semaphores are used to communicate events. Queues are 
 * used to communicate events, and to transfer data.
 * 
 * 2) Considerations When Using a Queue From an ISR
 * Queues provide an easy and convenient way of passing data from an interrupt 
 * to a task, but it is not efficient to use a queue if data is arriving at a high 
 * frequency. Unless the data is arriving slowing, it is recommended that 
 * production code does not copy the technique. More efficient techniques, that 
 * are suitable for production code, include: 
 * 
 * Using Direct Memory Access (DMA) hardware to receive and buffer characters. 
 * This method has practically no software overhead. A direct to task notification 
 * can then be used to unblock the task that will process the buffer only after a 
 * break in transmission has been detected.
 * 
 * NOTE: Direct to task notifications provide the most efficient method of unblocking 
 * a task from an ISR. Direct to task notifications are covered in Chapter 9.
 * 
 * Copying each received character into a thread safe RAM buffer. Again, a direct 
 * to task notification can be used to unblock the task that will process the 
 * buffer after a complete message has been received, or after a break in transmission 
 * has been detected.
 * 
 * NOTE: The ‘Stream Buffer’, provided as part of FreeRTOS+TCP 
 * (http://www.FreeRTOS.org/tcp), can be used for this purpose.
 * 
 * Processing the received characters directly within the ISR, then using a queue 
 * to send just the result of processing the data (rather than the raw data) to a 
 * task.
 * 
 * 2) With regards to portYIELD_FROM_ISR()
 * The interrupt will always return to the task in the Running state, even if the 
 * task in the Running state changed while the interrupt was executing.
 * 
 *  
 *******************************************************************************/

#define GPIO_PIN        9
#define TX_PERIOD_MS    2500

QueueHandle_t intQueue;
QueueHandle_t stringQueue;
BaseType_t full = false;
uint32_t ctr = 0;

void periodic_NumToISR_task(void *pvParameter)
{
    TickType_t prevTime = xTaskGetTickCount();
    while(1)
    {
        vTaskDelayUntil( &prevTime, pdMS_TO_TICKS(TX_PERIOD_MS) );
        if (xQueueSendToBack( intQueue, &ctr, 0 ) != errQUEUE_FULL)
        {
            printf("%d added to numQueue\r\n", ctr);
            if (++ctr >= 5){ctr = 0;}
        }
    }
}

void stringFromISR_task(void *pvParameter)
{
    char *rxString;
    while(1)
    {
        /* Block on the queue to wait for data to arrive. */
        xQueueReceive( stringQueue, &rxString, portMAX_DELAY );
        
        /* Print out the string received */
        printf("%s", rxString);
    }
}

void gpio_callback(uint gpio, uint32_t events)
{
    uint32_t uReceivedNumber = 0;

    // Defines an array of pointers to strings
    static const char *strings[] =
    {
        "String0\r\n",
        "String1\r\n",
        "String2\r\n",
        "String3\r\n",
        "String4\r\n"
    };
    
    /* As always, xHigherPriorityTaskWoken is initialized to pdFALSE to be able to
    detect it getting set to pdTRUE inside an interrupt safe API function. Note that
    as an interrupt safe API function can only set xHigherPriorityTaskWoken to
    pdTRUE, it is safe to use the same xHigherPriorityTaskWoken variable in both
    the call to xQueueReceiveFromISR() and the call to xQueueSendToBackFromISR(). */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    printf("ISR triggered\r\n");

    // event is an enum gpio_irq_level
    if (gpio == GPIO_PIN)
    {
        /* Read from the queue until the queue is empty. */
        while (xQueueReceiveFromISR(intQueue, 
                                    &uReceivedNumber,
                                    &xHigherPriorityTaskWoken) != errQUEUE_EMPTY)
        {
            printf("%d removed to numQueue\r\n", uReceivedNumber);
            xQueueSendToBackFromISR(stringQueue,
                                    &strings[uReceivedNumber],
                                    &xHigherPriorityTaskWoken );
        }
        ctr = 0;
    } // if (gpio == GPIO_PIN)

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

int main()
{
    stdio_init_all();
    while(!stdio_usb_connected()){tight_loop_contents();}

    sleep_ms(1000);
    printf("ISR Queues Example\r\n");
    gpio_pull_up(GPIO_PIN);
    gpio_set_irq_enabled_with_callback(GPIO_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    /* Before a queue can be used it must first be created.  Create both queues used
    by this example.  One queue can hold variables of type uint32_t, the other queue
    can hold variables of type char*.  Both queues can hold a maximum of 10 items.  A
    real application should check the return values to ensure the queues have been
    successfully created. */
    intQueue = xQueueCreate( 5, sizeof( uint32_t ) );
    stringQueue = xQueueCreate( 5, sizeof( char * ) );

    /* Create the task that uses a queue to pass integers to the interrupt service
    routine.  The task is created at priority 1. */
    xTaskCreate( periodic_NumToISR_task, "IntGen", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    
    /* Create the task that prints out the strings sent to it from the interrupt
    service routine.  This task is created at the higher priority of 2. */
    xTaskCreate( stringFromISR_task, "String", configMINIMAL_STACK_SIZE, NULL, 2, NULL );

    /* Start the scheduler so the created tasks start executing. */
    vTaskStartScheduler();

    /* As normal, the following line should never be reached. */
    for( ;; );
}