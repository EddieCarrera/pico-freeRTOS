#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *********************************
 * 1) The methods described so far have required the creation of a communication 
 * objects - queues, event groups, and various different types of semaphore. 
 * When a communication object is used, events and data are not sent or received 
 * directly to a receiving task, or a receiving ISR, but are instead sent to the 
 * communication object. 
 * 
 * Task Notifications allow tasks to interact with other tasks, and to synchronize 
 * with ISRs, without the need for a separate communication object.
 * 
 * Task notification functionality is optional. To include task notification 
 * functionality set configUSE_TASK_NOTIFICATIONS to 1 in FreeRTOSConfig.h. 
 * 
 * When configUSE_TASK_NOTIFICATIONS is set to 1, each task has a ‘Notification State’,
 * which can be either ‘Pending’ or ‘Not-Pending’, and a ‘Notification Value’, which 
 * is a 32-bit unsigned integer. When a task receives a notification, its notification 
 * state is set to pending. When a task reads its notification value, its notification 
 * state is set to not-pending. A task can wait in the Blocked state, with an optional 
 * time out, for its notification state to become pending.
 * 
 * 2) Benefits of Task Notifications 
 * A . Performance
 * ********************
 * Using a task notification to send an event or data to a task is 
 * significantly faster than using a communication object
 * 
 * B RAM improvement
 * *********************
 * Communication objects be created before use and therefore 
 * consume more RAM. Enabling task notification functionality has a fixed overhead
 * of just eight bytes of RAM per task.
 * 
 * 3) Limitations of Task Notifications
 * Task notifications are faster and use less RAM than communication objects, but 
 * task notifications CANNOT be used in all scenarios:
 * 
 * A. Sending an event or data to an ISR
 * ******************************************
 * Communication objects can be used to send events and data from an ISR to a task, 
 * and from a task to an ISR. 
 *
 * Task notifications can be used to send events and data from an ISR to a task, 
 * but they cannot be used to send events or data from a task to an ISR.
 * 
 * B. Enabling more than one receiving task
 * *******************************************
 * A communication object can be accessed by any task or ISR that knows its handle 
 * (queue, semaphore, or event group handle). Any number of tasks and ISRs can 
 * process events or data sent to any given communication object.
 * 
 * Task notifications are sent directly to the receiving task, so can only be processed 
 * by the task to which the notification is sent. However, this is rarely a limitation 
 * in practical cases because, while it is common to have multiple tasks and ISRs sending 
 * to the same communication object, it is rare to have multiple tasks and ISRs receiving 
 * from the same communication object.
 * 
 * C. Buffering multiple data items
 * *************************************
 * A queue is a communication object that can hold more than one data item at a time. 
 * Data that has been sent to the queue, but not yet received from the queue, is 
 * buffered inside the queue object.
 * 
 * Task notifications send data to a task by updating the receiving task’s notification 
 * value. A task’s notification value can only hold one value at a time.
 * 
 * D. Broadcasting to more than one task
 * ****************************************
 * An event group is a communication object that can be used to send an event to 
 * more than one task at a time.
 * 
 * Task notifications are sent directly to the receiving task, so can only be 
 * processed by the receiving task
 * 
 * E. Waiting in the blocked state for a send to complete
 * **********************************************************
 * If a communication object is temporarily in a state that means no more data 
 * or events can be written to it (for example, when a queue is full no more data 
 * can be sent to the queue), then tasks attempting to write to the object can 
 * optionally enter the Blocked state to wait for their write operation to complete. 
 * 
 * If a task attempts to send a task notification to a task that already has a 
 * notification pending, then it is not possible for the sending task to wait in 
 * the Blocked state for the receiving task to reset its notification state. As 
 * will be seen, this is rarely a limitation in practical cases in which a task 
 * notification is used.
 * 
 * 4) Task notifications are a very powerful feature that can often be used in 
 * place of a binary semaphore, a counting semaphore, an event group, and sometimes 
 * even a queue. This wide range of usage scenarios can be achieved by using the 
 * xTaskNotify() API function to send a task notification, and the xTaskNotifyWait() 
 * API function to receive a task notification.
 * 
 * However, in the majority of cases, the full flexibility provided by the xTaskNotify() 
 * and xTaskNotifyWait() API functions is not required, and simpler functions would 
 * suffice. Therefore, the xTaskNotifyGive() API function is provided as a simpler but 
 * less flexible alternative to xTaskNotify(), and the ulTaskNotifyTake() API function 
 * is provided as a simpler but less flexible alternative to xTaskNotifyWait().
 * 
 * 5) The best uses of xTaskNotify() can be found on page 308. It contains a 
 *  eNotifyAction parameter that allows for the following actions:
 * 
 * A. eNoAction
 * 
 * B. eSetBits
 * 
 * C.
 *******************************************************************************/

#define GPIO_PIN 9

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
        /* Send a notification directly to the task to which interrupt processing is
        being deferred. */
        vTaskNotifyGiveFromISR( /* The handle of the task to which the notification
                                   is being sent. The handle was saved when the task
                                   was created. */
                                gpio_triggered_task,
                                /* xHigherPriorityTaskWoken is used in the usual way. */
                                &xHigherPriorityTaskWoken );

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
        /* Wait to receive a notification sent directly to this task from the
        interrupt service routine. */

        /* xClearCountOnExit parameter can be changed to pdFALSE so that 
        ulTaskNotifyTake behaves more like a counting semaphore */
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
        printf( "Handler task - Processing event.\r\n");
    }
}

int main()
{
    stdio_init_all();
    while(!stdio_usb_connected()){tight_loop_contents();}

    sleep_ms(1000);
    printf("Interrupt task notification example\r\n");
    gpio_pull_up(GPIO_PIN);
    gpio_set_irq_enabled_with_callback(GPIO_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    /* Create the 'handler' task, which is the task to which interrupt
    processing is deferred.  This is the task that will be synchronized with
    the interrupt.  The handler task is created with a high priority to ensure
    it runs immediately after the interrupt exits.  In this case a priority of
    3 is chosen. */
    xTaskCreate(gpio_triggered_task, "Handler", configMINIMAL_STACK_SIZE, NULL, 3, NULL );

    /* Start the scheduler so the created tasks start executing. */
    vTaskStartScheduler();

    /* As normal, the following line should never be reached. */
    for( ;; );
}