#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *********************************
 * 1) Blocking on Queue Reads
 * When a task attempts to read from a queue, it can optionally specify 
 * a ‘block’ time. This is the time the task will be kept in the Blocked 
 * state to wait for data to be available from the queue, should the queue 
 * already be empty. A task that is in the Blocked state, waiting for data 
 * to become available from a queue, is automatically moved to the Ready 
 * state when another task or interrupt places data into the queue. The 
 * task will also be moved automatically from the Blocked state to the 
 * Ready state if the specified block time expires before data becomes 
 * available.
 * 
 * Queues can have multiple readers, so it is possible for a single queue to 
 * have more than one task blocked on it waiting for data. When this is the 
 * case, only one task will be unblocked when data becomes available. The 
 * task that is unblocked will always be the highest priority task that is 
 * waiting for data. If the blocked tasks have equal priority, then the task 
 * that has been waiting for data the longest will be unblocked.
 * 
 * 2) Blocking on Queue Writes
 * Just as when reading from a queue, a task can optionally specify a block 
 * time when writing to a queue. In this case, the block time is the maximum 
 * time the task should be held in the Blocked state to wait for space to become 
 * available on the queue, should the queue already be full.
 * 
 * Queues can have multiple writers, so it is possible for a full queue to have 
 * more than one task blocked on it waiting to complete a send operation. When 
 * this is the case, only one task will be unblocked when space on the queue 
 * becomes available. The task that is unblocked will always be the highest 
 * priority task that is waiting for space. If the blocked tasks have equal 
 * priority, then the task that has been waiting for space the longest will be 
 * unblocked.
 *******************************************************************************/


/* Enums */
typedef enum {
    sender1,
    sender2
} dataSource_t;

/* Struct */
typedef struct {
    uint8_t value;
    dataSource_t source;
} data_t;

data_t dataToSend[2] =  { {100, sender1},
                          {200, sender2}
                        };

QueueHandle_t queue;

#define SEND_TASK_PRIORITY      2
#define RECEIVE_TASK_PRIORITY   1   

// Continuous task
void transmitTask(void *parameter)
{  
    BaseType_t status;
    while(1)
    {
        status = xQueueSendToBack(queue, (data_t*)parameter, pdMS_TO_TICKS(100));

        if (status == pdFAIL)
        {
            printf("Task 1 could not add to queue\r\n");
        }
    }
}

void receiveTask(void *parameter)
{
    BaseType_t status;
    data_t rx;

    while(1)
    {
        if (uxQueueMessagesWaiting(queue) != 5)
        {
            printf("Queue should have been full!\r\n");
        }
        status = xQueueReceive(queue, &rx, 0);
        if (status == pdPASS)
        {
            if (rx.source == sender1)
            {
                printf("From sender1: %d\r\n", rx.value);
            }
            else if (rx.source == sender2)
            {
                printf("From sender2: %d\r\n", rx.value);
            }
        }
        else {
            printf("Could not receive from queuer\r\n");
        }
    }
}

int main()
{
    queue = xQueueCreate(5, sizeof(data_t));

    stdio_init_all();

    xTaskCreate(transmitTask, "Transmit1", configMINIMAL_STACK_SIZE, 
                &(dataToSend[0]),SEND_TASK_PRIORITY, NULL);
    xTaskCreate(transmitTask, "Transmit2", configMINIMAL_STACK_SIZE, 
                &(dataToSend[1]), SEND_TASK_PRIORITY, NULL);    
    xTaskCreate(receiveTask, "ReceiveTask", configMINIMAL_STACK_SIZE, 
                NULL, RECEIVE_TASK_PRIORITY, NULL);

    vTaskStartScheduler();

 /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks.  If main() does reach here then it is likely that
    there was insufficient heap memory available for the idle task to be created */
    printf("ERROR\r\n");
    while(1);
}
