#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include "pico/stdlib.h"
#include "pico/rand.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *********************************
 * 1) Although mutexes are useful, precautions must be taken to avoid several 
 * pitfalls associated with them:
 * 
 ****************************
 * A. Priority Inversion  
 ****************************
 * Consider the case when (1) the LP task takes a mutex before being preempted 
 * by the HP task. (2) The HP task attempts to take the mutex but can’t because 
 * it is still being held by the LP task. The HP task enters the Blocked state to 
 * wait for the mutex to become available. (3) The LP task continues to execute, 
 * but gets preempted by the MP task before it gives the mutex back. 
 * (4) The MP task is now running. The HP task is still waiting for the LP task 
 * to return the mutex, but the LP task is not even executing! 
 * 
 * Priority inversion can be a significant problem, but in small embedded systems 
 * it can often be avoided at system design time, by considering how resources are 
 * accessed. 
 * 
 * Priority Inheritance 
 * FreeRTOS mutexes and binary semaphores are very similar—the difference being 
 * that mutexes include a basic ‘priority inheritance’ mechanism, whereas binary 
 * semaphores do not. Priority inheritance does not ‘fix’ priority inversion, but 
 * merely lessens its impact by ensuring that the inversion is always time bounded. 
 * However, priority inheritance complicates system timing analysis, and it is not 
 * good practice to rely on it for correct system operation.
 * 
 * Priority inheritance works by temporarily raising the priority of the mutex 
 * holder to the priority of the highest priority task that is attempting to obtain 
 * the same mutex. The low priority task that holds the mutex ‘inherits’ the 
 * priority of the task waiting for the mutex. The following is how inheritance 
 * resolves the case considered above:
 * 
 * (1) The LP task takes a mutex before being preempted by the HP task. (2) The HP 
 * task attempts to take the mutex but can’t because it is still being held by the 
 * LP task. The HP task enters the Blocked state to wait for the mutex to become 
 * available. (3) The LP task is preventing the HP task from executing so inherits 
 * the priority of the HP task. The LP task cannot now be preempted by the MP task, 
 * so the amount of time that priority inversion exists is minimized. When the LP 
 * task gives the mutex back it returns to its original priority. (4) The LP task 
 * returning the mutex causes the HP task to exit the Blocked state as the mutex 
 * holder. When the HP task has finished with the mutex it gives it back. 
 * The MP task only executes when the HP task returns to the Blocked state so the 
 * MP task never holds up the HP task.
 * 
 **************************************
 * B. Deadlock (or deadly embrace)  
 **************************************
 * 
 * Deadlock occurs when two tasks cannot proceed because they are both waiting for a 
 * resource that is held by the other. Consider the following scenario: (1) Task A 
 * executes and successfully takes mutex X. (2) Task A is preempted by Task B. 
 * (3) Task B successfully takes mutex Y before attempting to also take mutex X, 
 * but mutex X is held by Task A so is not available to Task B. Task B opts to enter 
 * the Blocked state to wait for mutex X to be released. (4) Task A continues 
 * executing. It attempts to take mutex Y—but mutex Y is held by Task B, so is not 
 * available to Task A. Task A opts to enter the Blocked state to wait for mutex Y 
 * to be released. 
 * 
 * At the end of this scenario, Task A is waiting for a mutex held by 
 * Task B, and Task B is waiting for a mutex held by Task A. Deadlock has occurred 
 * because neither task can proceed.
 * 
 * As with priority inversion, the best method of avoiding deadlock is to consider 
 * its potential at design time, and design the system to ensure that deadlock cannot 
 * occur. In particular, it is normally bad practice for a task to wait indefinitely 
 * (without a time out) to obtain a mutex. Instead, use a time out that is a little 
 * longer than the maximum time it is expected to have to wait for the mutex—then failure 
 * to obtain the mutex within that time will be a symptom of a design error, which might 
 * be a deadlock.
 * 
 * In practice, deadlock is not a big problem in small embedded systems, because the 
 * system designers can have a good understanding of the entire application, and so 
 * can identify and remove the areas where it could occur.
 * 
 ****************************
 * C. Recursive Mutexes
 ****************************
 * It is also possible for a task to deadlock with itself. This will happen if a task 
 * attempts to take the same mutex more than once, without first returning the mutex. 
 * Consider the following scenario: (1) A task successfully obtains a mutex. (2) While 
 * holding the mutex, the task calls a library function. (3) The implementation of the 
 * library function attempts to take the same mutex, and enters the Blocked state to 
 * wait for the mutex to become available. At the end of this scenario the task is in 
 * the Blocked state to wait for the mutex to be returned, but the task is already the 
 * mutex holder. A deadlock has occurred because the task is in the Blocked state to wait 
 * for itself. 
 * 
 * This type of deadlock can be avoided by using a recursive mutex in place of a standard 
 * mutex. A recursive mutex can be ‘taken’ more than once by the same task, and will be 
 * returned only after one call to ‘give’ the recursive mutex has been executed for every 
 * preceding call to ‘take’ the recursive mutex.
 * 
 *********************************
 * D. Mutexes & Task Scheduling
 *********************************
 * It is common to make an incorrect assumption as to the order in which the tasks will 
 * execute when the tasks have the same priority. If Task 1 and Task 2 have the same 
 * priority, and Task 1 is in the Blocked state to wait for a mutex that is held by 
 * Task 2, then Task 1 will not preempt Task 2 when Task 2 ‘gives’ the mutex. Instead, 
 * Task 2 will remain in the Running state, and Task 1 will simply move from the Blocked 
 * state to the Ready state. If Task 2 takes the the mutex again before the next RTOS 
 * tick, then it's possible for Task 1 to be blocked and for Task 2 to consume all the
 * processing time. 
 * 
 * This scenario can be avoided by adding a call to taskYIELD() after the call
 * to xSemaphoreGive() by Task 2.
 * 
 * Refer to Figure 68 & 69 of "Mastering the FreeRTOS" manual (page 256)
 * 
 * 2) Gatekeeper Tasks
 * Gatekeeper tasks provide a clean method of implementing mutual exclusion without the 
 * risk of priority inversion or deadlock. A gatekeeper task is a task that has sole 
 * ownership of a resource. Only the gatekeeper task is allowed to access the resource 
 * directly—any other task needing to access the resource can do so only indirectly by 
 * using the services of the gatekeeper
 * 
 * In the example below, the gatekeeper task uses a FreeRTOS queue to serialize access 
 * to standard out. The internal implementation of the task does not have to consider 
 * mutual exclusion because it is the only task permitted to access standard out directly.

 * 
 *******************************************************************************/

SemaphoreHandle_t xMutex;
QueueHandle_t xPrintQueue;

/* Define the strings that the tasks and interrupt will print out via the
gatekeeper. */
static char *pcStringsToPrint[] =
{
 "Task 1 ****************************************************\r\n",
 "Task 2 ----------------------------------------------------\r\n",
 "Message printed from the tick hook interrupt ##############\r\n"
};

/* Tick hook functions execute within the context of the tick interrupt, and so must be kept very
short, must use only a moderate amount of stack space, and must not call any FreeRTOS API
functions that do not end with ‘FromISR()’. 

The tick hook function counts the number of times it is called, sending its message to the
gatekeeper task each time the count reaches 200. For demonstration purposes only, the tick
hook writes to the front of the queue, and the tasks write to the back of the queue. */
void vApplicationTickHook( void )
{
    static int iCount = 0;
    /* Print out a message every 200 ticks. The message is not written out directly,
    but sent to the gatekeeper task. */
    iCount++;
    if( iCount >= 200 )
    {
        /* As xQueueSendToFrontFromISR() is being called from the tick hook, it is
        not necessary to use the xHigherPriorityTaskWoken parameter (the third
        parameter), and the parameter is set to NULL. */
        xQueueSendToFrontFromISR( xPrintQueue,
        &( pcStringsToPrint[ 2 ] ),
        NULL );

        /* Reset the count ready to print out the string again in 200 ticks time. */
        iCount = 0;
    }
}

static void prvStdioGatekeeperTask( void *pvParameters )
{
    char *pcMessageToPrint;
    /* This is the only task that is allowed to write to serial. Any other
    task wanting to write a string to the output does not access standard out
    directly, but instead sends the string to this task. As only this task accesses
    standard out there are no mutual exclusion or serialization issues to consider
    within the implementation of the task itself. */
    while(1)
    {
        /* Wait for a message to arrive. An indefinite block time is specified so
        there is no need to check the return value – the function will only return
        when a message has been successfully received. */
        xQueueReceive( xPrintQueue, &pcMessageToPrint, portMAX_DELAY );
        /* Output the received string. */
        uint32_t len = strlen(pcMessageToPrint);
        uint32_t idx = 0;
        for (idx = 0; idx < len; idx++){
            putchar(*(pcMessageToPrint+idx));
        }
        /* Loop back to wait for the next message. */
    }
}

static void prvPrintTask( void *pvParameters )
{
    int iIndexToString;
    const TickType_t xMaxBlockTimeTicks = 0x20;
    /* Two instances of this task are created. The task parameter is used to pass
    an index into an array of strings into the task. Cast this to the required type. */
    iIndexToString = ( int ) pvParameters;
    while(1)
    {
        /* Print out the string, not directly, but instead by passing a pointer to
        the string to the gatekeeper task via a queue. The queue is created before
        the scheduler is started so will already exist by the time this task executes
        for the first time. A block time is not specified because there should
        always be space in the queue. */
        xQueueSendToBack( xPrintQueue, &( pcStringsToPrint[ iIndexToString ] ), 0 );
        /* Wait a pseudo random time. Note that rand() is not necessarily reentrant,
        but in this case it does not really matter as the code does not care what
        value is returned. In a more secure application a version of rand() that is
        known to be reentrant should be used - or calls to rand() should be protected
        using a critical section. */
        vTaskDelay( ( get_rand_32() % xMaxBlockTimeTicks ) );
    }
}


int main()
{
    stdio_init_all();
    while(!stdio_usb_connected()){tight_loop_contents();}

    sleep_ms(1000);
    printf("Gatekeeper Task example\r\n");

    /* Before a queue is used it must be explicitly created. The queue is created
    to hold a maximum of 5 character pointers. */
    xPrintQueue = xQueueCreate( 5, sizeof( char * ) );

    /* Check the queue was created successfully. */
    if( xPrintQueue != NULL )
    {
        /* Create two instances of the tasks that send messages to the gatekeeper.
        The index to the string the task uses is passed to the task via the task
        parameter (the 4th parameter to xTaskCreate()). The tasks are created at
        different priorities so the higher priority task will occasionally preempt
        the lower priority task. */
        xTaskCreate( prvPrintTask, "Print1", configMINIMAL_STACK_SIZE, ( void * ) 0, 1, NULL );
        xTaskCreate( prvPrintTask, "Print2", configMINIMAL_STACK_SIZE, ( void * ) 1, 2, NULL );

        /* Create the gatekeeper task. This is the only task that is permitted
        to directly access standard out. 

        The gatekeeper task is assigned a lower priority than the print tasks—so messages 
        sent to the gatekeeper remain in the queue until both print tasks are in the Blocked 
        state. In some situations, it would be appropriate to assign the gatekeeper a higher 
        priority, so messages get processed immediately—but doing so would be at the cost of 
        the gatekeeper delaying lower priority tasks until it has completed accessing the protected 
        resource. */
        xTaskCreate( prvStdioGatekeeperTask, "Gatekeeper", configMINIMAL_STACK_SIZE, NULL, 0, NULL );

        /* Start the scheduler so the created tasks start executing. */
        vTaskStartScheduler();
    }

    /* If all is well then main() will never reach here as the scheduler will now be
    running the tasks. If main() does reach here then it is likely that there was
    insufficient heap memory available for the idle task to be created. Chapter 2
    provides more information on heap memory management. */
    for( ;; );

}