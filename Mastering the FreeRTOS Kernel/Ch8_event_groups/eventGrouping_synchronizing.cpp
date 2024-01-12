#include <FreeRTOS.h>
#include <task.h>
#include <event_groups.h>
#include "pico/rand.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *********************************
 * 1) Sometimes the design of an application requires two or more tasks to 
 * synchronize with each other. For example, consider a design where Task A 
 * receives an event, then delegates some of the processing necessitated by the 
 * event to three other tasks: Task B, Task C and Task D. If Task A cannot receive 
 * another event until tasks B, C and D have all completed processing the previous 
 * event, then all four tasks will need to synchronize with each other. Each task’s 
 * synchronization point will be after that task has completed its processing, and 
 * cannot proceed further until each of the other tasks have done the same. Task A 
 * can only receive another event after all four tasks have reached their 
 * synchronization point.
 * 
 * 2) An event group can be used to create a synchronization point: 
 * 
 *  - Each task that must participate in the synchronization is assigned a unique 
 *  event bit within the event group.
 * 
 *  - Each task sets its own event bit when it reaches the synchronization point.
 * 
 *  - Having set its own event bit, each task blocks on the event group to wait 
 *  for the event bits that represent all the other synchronizing tasks to also
 *  become set.
 * 
 * However, the xEventGroupSetBits() and xEventGroupWaitBits() API functions 
 * cannot be used in this scenario. If they were used, then the setting of a bit 
 * (to indicate a task had reached its synchronization point) and the testing of 
 * bits (to determine if the other synchronizing tasks had reached their synchronization 
 * point) would be performed as two separate operations. To see why that would be a 
 * problem, consider a scenario where Task A, Task B and Task C attempt to synchronize 
 * using an event group:
 * 
 * (1) Task A and Task B have already reached the synchronization point, so their 
 * event bits are set in the event group, and they are in the Blocked state to 
 * wait for task C’s event bit to also become set. (2) Task C reaches the synchronization 
 * point, and uses xEventGroupSetBits() to set its bit in the event group. As 
 * soon as Task C’s bit is set, Task A and Task B leave the Blocked state, and clear 
 * all three event bits. (3) Task C then calls xEventGroupWaitBits() to wait for all 
 * three event bits to become set, but by that time, all three event bits have already 
 * been cleared, Task A and Task B have left their respective synchronization points, 
 * and so the synchronization has failed.
 * 
 * To successfully use an event group to create a synchronization point, the setting 
 * of an event bit, and the subsequent testing of event bits, must be performed as a 
 * single uninterruptable operation. The xEventGroupSync() API function is provided 
 * for that purpose. The implementation for this is shown in the code below. 
 * 
 *******************************************************************************/
/* Definitions for the event bits in the event group. */
#define mainFIRST_TASK_BIT ( 1UL << 0UL ) /* Event bit 0, set by the first task. */
#define mainSECOND_TASK_BIT( 1UL << 1UL ) /* Event bit 1, set by the second task. */
#define mainTHIRD_TASK_BIT ( 1UL << 2UL ) /* Event bit 2, set by the third task. */

/* Declare the event group used to synchronize the three tasks. */
EventGroupHandle_t xEventGroup;

static void vSyncingTask( void *pvParameters )
{
    const TickType_t xMaxDelay = pdMS_TO_TICKS( 4000UL );
    const TickType_t xMinDelay = pdMS_TO_TICKS( 200UL );
    TickType_t xDelayTime;
    EventBits_t uxThisTasksSyncBit;
    const EventBits_t uxAllSyncBits = (mainFIRST_TASK_BIT|mainSECOND_TASK_BIT|mainTHIRD_TASK_BIT);

    /* Three instances of this task are created - each task uses a different event
    bit in the synchronization. The event bit to use is passed into each task
    instance using the task parameter. Store it in the uxThisTasksSyncBit
    variable. */
    uxThisTasksSyncBit = ( EventBits_t ) pvParameters;
    for( ;; )
    {
        /* Simulate this task taking some time to perform an action by delaying for a
        pseudo random time. This prevents all three instances of this task reaching
        the synchronization point at the same time, and so allows the example’s
        behavior to be observed more easily. */
        xDelayTime = ( get_rand_32() % xMaxDelay ) + xMinDelay;
        vTaskDelay( xDelayTime );

        /* Print out a message to show this task has reached its synchronization
        point. pcTaskGetTaskName() is an API function that returns the name assigned
        to the task when the task was created. */
        printf("%s%s", pcTaskGetTaskName( NULL ), " reached sync point\r\n" );

        /* Wait for all the tasks to have reached their respective synchronization
        points. */
        xEventGroupSync( /* The event group used to synchronize. */
        xEventGroup,
        /* The bit set by this task to indicate it has reached the
        synchronization point. */
        uxThisTasksSyncBit,
        /* The bits to wait for, one bit for each task taking part
        in the synchronization. */
        uxAllSyncBits,
        /* Wait indefinitely for all three tasks to reach the
        synchronization point. */
        portMAX_DELAY );
        /* Print out a message to show this task has passed its synchronization
        point. As an indefinite delay was used the following line will only be
        executed after all the tasks reached their respective synchronization
        points. */

        printf( pcTaskGetTaskName( NULL ), " exited sync point\r\n" );
    }
}

int main()
{
    stdio_init_all();
    while(!stdio_usb_connected()){tight_loop_contents();}

    sleep_ms(1000);
    printf("Event grouping synchronizing example\r\n");
    gpio_pull_up(GPIO_PIN);
    gpio_set_irq_enabled_with_callback(GPIO_PIN, GPIO_IRQ_EDGE_FALL, true, &ulEventBitSettingISR);

    /* Before an event group can be used it must first be created. */
    xEventGroup = xEventGroupCreate();

    /* Create three instances of the task. Each task is given a different name,
    which is later printed out to give a visual indication of which task is
    executing. The event bit to use when the task reaches its synchronization point
    is passed into the task using the task parameter. */
    xTaskCreate( vSyncingTask, "Task 1", configMINIMAL_STACK_SIZE, mainFIRST_TASK_BIT, 1, NULL );
    xTaskCreate( vSyncingTask, "Task 2", configMINIMAL_STACK_SIZE, mainSECOND_TASK_BIT, 1, NULL );
    xTaskCreate( vSyncingTask, "Task 3", configMINIMAL_STACK_SIZE, mainTHIRD_TASK_BIT, 1, NULL );

    /* Start the scheduler so the created tasks start executing. */
    vTaskStartScheduler();

    /* As always, the following line should never be reached. */
    for( ;; );
}