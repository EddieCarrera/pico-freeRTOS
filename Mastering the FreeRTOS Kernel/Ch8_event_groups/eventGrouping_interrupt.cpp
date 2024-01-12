#include <FreeRTOS.h>
#include <task.h>
#include <event_groups.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *********************************
 * 1) Event groups are a feature that allow events to be communicated to tasks.
 * Unlike queues and semaphores:
 * 
 * a. Event groups allow a task to wait in the Blocked state for a combination of 
 * one of more events to occur.
 * 
 * b. Event groups unblock all the tasks that were waiting for the same event, or 
 * combination of events, when the event occurs.
 * 
 * Event groups also provide the opportunity to reduce the RAM used by an application, 
 * as often it is possible to replace many binary semaphores with a single event group.
 * 
 * 2) An event ‘flag’ is a Boolean (1 or 0) bit value used to indicate if an event has 
 * occurred or not. An event ‘group’ is a set of event flags. 
 *******************************************************************************/
/* Definitions for the event bits in the event group. */
#define mainFIRST_TASK_BIT ( 1UL << 0UL ) /* Event bit 0, which is set by a task. */
#define mainSECOND_TASK_BIT ( 1UL << 1UL ) /* Event bit 1, which is set by a task. */
#define mainISR_BIT ( 1UL << 2UL ) /* Event bit 2, which is set by an ISR. */
#define GPIO_PIN 9

/* Declare the event group used to synchronize the three tasks. */
EventGroupHandle_t xEventGroup;

void vDeferredHandlingFunction( void *pvParameter, uint32_t ulParameter2 )
{
    printf("%s", pvParameter);
}

static void gpio_callback(uint gpio, uint32_t events)
{
    /* The string is not printed within the interrupt service routine, but is instead
    sent to the RTOS daemon task for printing. It is therefore declared static to ensure
    the compiler does not allocate the string on the stack of the ISR, as the ISR's stack
    frame will not exist when the string is printed from the daemon task. */
    static const char *pcString = "Bit setting ISR -\t about to set bit 2.\r\n";

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    /* Print out a message to say bit 2 is about to be set. Messages cannot be
    printed from an ISR, so defer the actual output to the RTOS daemon task by
    pending a function call to run in the context of the RTOS daemon task. */
    xTimerPendFunctionCallFromISR( vDeferredHandlingFunction,
    ( void * ) pcString,
    0,
    &xHigherPriorityTaskWoken );

    /* Set bit 2 in the event group. */
    xEventGroupSetBitsFromISR( xEventGroup, mainISR_BIT, &xHigherPriorityTaskWoken );

    /* xTimerPendFunctionCallFromISR() and xEventGroupSetBitsFromISR() both write to
    the timer command queue, and both used the same xHigherPriorityTaskWoken
    variable. If writing to the timer command queue resulted in the RTOS daemon task
    leaving the Blocked state, and if the priority of the RTOS daemon task is higher
    than the priority of the currently executing task (the task this interrupt
    interrupted) then xHigherPriorityTaskWoken will have been set to pdTRUE.
    
    xHigherPriorityTaskWoken is used as the parameter to portYIELD_FROM_ISR(). If
    xHigherPriorityTaskWoken equals pdTRUE, then calling portYIELD_FROM_ISR() will
    request a context switch. If xHigherPriorityTaskWoken is still pdFALSE, then
    calling portYIELD_FROM_ISR() will have no effect. */
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

static void vEventBitSettingTask( void *pvParameters )
{
    const TickType_t xDelay200ms = pdMS_TO_TICKS( 200UL ), xDontBlock = 0;
    for( ;; )
    {
        /* Delay for a short while before starting the next loop. */
        vTaskDelay( xDelay200ms );

        /* Print out a message to say event bit 0 is about to be set by the task,
        then set event bit 0. */
        printf( "Bit setting task -\t about to set bit 0.\r\n" );

        xEventGroupSetBits( xEventGroup, mainFIRST_TASK_BIT );

        /* Delay for a short while before setting the other bit. */
        vTaskDelay( xDelay200ms );
        
        /* Print out a message to say event bit 1 is about to be set by the task,
        then set event bit 1. */
        printf( "Bit setting task -\t about to set bit 1.\r\n" );
        xEventGroupSetBits( xEventGroup, mainSECOND_TASK_BIT );
    }
}

static void vEventBitReadingTask( void *pvParameters )
{
    EventBits_t xEventGroupValue;
    const EventBits_t xBitsToWaitFor = ( mainFIRST_TASK_BIT | mainSECOND_TASK_BIT | mainISR_BIT );
    for( ;; )
    {
        /* Block to wait for event bits to become set within the event group. */
        xEventGroupValue = xEventGroupWaitBits( /* The event group to read. */
        xEventGroup,
        /* Bits to test. */
        xBitsToWaitFor,
        /* Clear bits on exit if the
        unblock condition is met. */
        pdTRUE,
        /* Don't wait for all bits. This
        parameter is set to pdTRUE for the
        second execution. */
        pdFALSE,                    // Play with this parameter to see xEventGroupWaitBits() 
        /* Don't time out. */       // behavior change
        portMAX_DELAY );

        /* Print a message for each bit that was set. */
        if (xEventGroupValue & mainFIRST_TASK_BIT != 0)
        {
            printf( "Bit reading task -\t Event bit 0 was set\r\n");
        }
        if (xEventGroupValue & mainSECOND_TASK_BIT != 0)
        {
            printf( "Bit reading task -\t Event bit 1 was set\r\n");
        }
        if (xEventGroupValue & mainISR_BIT != 0)
        {
            printf( "Bit reading task -\t Event bit 2 was set\r\n");
        }
    }
}

int main()
{
    stdio_init_all();
    while(!stdio_usb_connected()){tight_loop_contents();}

    sleep_ms(1000);
    printf("Event grouping example\r\n");
    gpio_pull_up(GPIO_PIN);
    gpio_set_irq_enabled_with_callback(GPIO_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    /* Before an event group can be used it must first be created. */
    xEventGroup = xEventGroupCreate();

    /* Create the task that sets event bits in the event group. */
    xTaskCreate( vEventBitSettingTask, "Bit Setter", configMINIMAL_STACK_SIZE, NULL, 1, NULL );

    /* Create the task that waits for event bits to get set in the event group. */
    xTaskCreate( vEventBitReadingTask, "Bit Reader", configMINIMAL_STACK_SIZE, NULL, 2, NULL );

    /* Start the scheduler so the created tasks start executing. */
    vTaskStartScheduler();
    
    /* The following line should never be reached. */
    for( ;; );
}