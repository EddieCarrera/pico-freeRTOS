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
 *******************************************************************************/

#define GPIO_PIN        9
#define TX_PERIOD_MS    5000

QueueHandle_t intQueue;
QueueHandle_t stringQueue;
BaseType_t full = false;

void periodic_NumToISR_task(void *pvParameter)
{
    static uint32_t ctr = 0;
    TickType_t prevTime = xTaskGetTickCount();

    while(1)
    {
        vTaskDelayUntil( &prevTime, pdMS_TO_TICKS(TX_PERIOD_MS) );
        if (xQueueSendToBack( intQueue, &ctr, 0 ) != errQUEUE_FULL)
        {
            ctr = ctr > 5 ? ctr = 0 : ctr++;
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
        "String1\r\n",
        "String2\r\n",
        "String3\r\n",
        "String4\r\n"
        "String5\r\n"
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
        while (xQueueReceiveFromISR(xIntegerQueue, 
                                    &uReceivedNumber,
                                    &xHigherPriorityTaskWoken) != errQUEUE_EMPTY)
        {
            xQueueSendToBackFromISR(xStringQueue,
                                    &strings[uReceivedNumber],
                                    &xHigherPriorityTaskWoken );
        }
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

    /* Start the scheduler so the created tasks start executing. */
    vTaskStartScheduler();

    /* As normal, the following line should never be reached. */
    for( ;; );
}