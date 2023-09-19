#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <semphr.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *********************************
 * 1) It is possible to use the xTimerPendFunctionCallFromISR() API function to 
 * defer interrupt processing to the RTOS daemon task—removing the need to create 
 * a separate task for each interrupt. Deferring interrupt processing to the daemon 
 * task is called ‘centralized deferred interrupt processing’.
 * 
 * The xTimerPendFunctionCall() and xTimerPendFunctionCallFromISR() API functions 
 * use the same timer command queue to send an ‘execute function’ command to the 
 * daemon task. The function sent to the daemon task is then executed in the context 
 * of the daemon task.
 * 
 * Advantages of centralized deferred interrupt processing include:
 * Lower resource usage:  It removes the need to create a separate task for each 
 *                        deferred interrupt.
 * Simplified user model: The deferred interrupt handling function is a standard 
 *                        C function.
 * 
 * Disadvantages of centralized deferred interrupt processing include:
 * Less flexibility: Each deferred interrupt handling function executes at the 
 *                   priority of the daemon task. As described in Chapter 5, the 
 *                   priority of the daemon task is set by the configTIMER_TASK_PRIORITY 
 *                   within FreeRTOSConfig.h.
 * Less determinism: xTimerPendFunctionCallFromISR() sends a command to the back of 
 *                   the timer command queue. Commands that were already in the timer 
 *                   command queue will be processed by the daemon task before the 
 *                   ‘execute function’ command sent to the queue by 
 *                   xTimerPendFunctionCallFromISR().
 * 
 * 2) In the example below, notice how main() is more simpler because it doesn't create
 * a semaphore or a task to perform deferred interrupt processing.
 *******************************************************************************/


#define GPIO_PIN 9

void vDeferredHandlingFunction( void *pvParameter1, uint32_t ulParameter2 )
{
    /* Process the event - in this case just print out a message and the value of
    ulParameter2. pvParameter1 is not used in this example. */
    printf("Handler task - Processing event %d\r\n", ulParameter2);
}

void gpio_callback(uint gpio, uint32_t events)
{
    static uint32_t ulParameterValue = 0;

    /* The xHigherPriorityTaskWoken parameter must be initialized to pdFALSE as
    it will get set to pdTRUE inside the interrupt safe API function if a
    context switch is required. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    printf("ISR triggered\r\n");

    // event is an enum gpio_irq_level
    if (gpio == GPIO_PIN)
    {

        /* Send a pointer to the interrupt's deferred handling function to the daemon task.
        The deferred handling function's pvParameter1 parameter is not used so just set to
        NULL. The deferred handling function's ulParameter2 parameter is used to pass a
        number that is incremented by one each time this interrupt handler executes. */
        xTimerPendFunctionCallFromISR( 
        vDeferredHandlingFunction,  /* Function to execute. */
        NULL,                       /* Not used. */
        ulParameterValue,           /* Incrementing value. */
        &xHigherPriorityTaskWoken );

        ulParameterValue++;

        /* Pass the xHigherPriorityTaskWoken value into portYIELD_FROM_ISR().  If
        xHigherPriorityTaskWoken was set to pdTRUE inside xSemaphoreGiveFromISR()
        then calling portYIELD_FROM_ISR() will request a context switch.  If
        xHigherPriorityTaskWoken is still pdFALSE then calling
        portYIELD_FROM_ISR() will have no effect. */
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

int main()
{
    stdio_init_all();
    while(!stdio_usb_connected()){tight_loop_contents();}

    sleep_ms(1000);
    printf("Defer to daemon task example\r\n");
    gpio_pull_up(GPIO_PIN);
    gpio_set_irq_enabled_with_callback(GPIO_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    /* Start the scheduler so the created tasks start executing. */
    vTaskStartScheduler();

    /* As normal, the following line should never be reached. */
    for( ;; );
}