#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "pico/stdlib.h"
#include "pico/rand.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *********************************
 * 1) A Mutex is a special type of binary semaphore that is used to control 
 * access to a resource that is shared between two or more tasks.
 * 
 * When used in a mutual exclusion scenario, the mutex can be thought of as a 
 * token that is associated with the resource being shared. For a task to access 
 * the resource legitimately, it must first successfully ‘take’ the token (be the 
 * token holder). When the token holder has finished with the resource, it must 
 * ‘give’ the token back. Only when the token has been returned can another task 
 * successfully take the token, and then safely access the same shared resource. 
 * A task is not permitted to access the shared resource unless it holds the token. 
 * 
 * 2) Even though mutexes and binary semaphores share many characteristics, 
 * there are differences. The primary difference is what happens to the semaphore 
 * after it has been obtained:
 *  - A semaphore that is used for mutual exclusion must always be returned.
 *  - A semaphore that is used for synchronization is normally discarded and 
 *    not returned.
 * 
 * 3) Notice that a looped putchar() block is used instead of printf() to 
 * allow for the competing tasks to corrupt each others data and demonstrate 
 * the usefulness of a mutex. 
 * 
 *******************************************************************************/

SemaphoreHandle_t xMutex;

static void prvNewPrintString( const char *pcString )
{
    /* The mutex is created before the scheduler is started, so already exists by the
    time this task executes.

    Attempt to take the mutex, blocking indefinitely to wait for the mutex if it is
    not available straight away. The call to xSemaphoreTake() will only return when
    the mutex has been successfully obtained, so there is no need to check the
    function return value. If any other delay period was used then the code must
    check that xSemaphoreTake() returns pdTRUE before accessing the shared resource
    (which in this case is standard out). As noted earlier in this book, indefinite
    time outs are not recommended for production code. */
    xSemaphoreTake(xMutex, portMAX_DELAY);
    {
        /* The following line will only execute once the mutex has been successfully
        obtained. Standard out can be accessed freely now as only one task can have
        the mutex at any one time. */

        /* printf() not used because it's sequential and doesn't allow for the 
        two tasks to compete for the serial resource. putchar() is used instead
        because it allows for corruption to demonstrate the application of a mutex */
        
        // printf( "%s", pcString );

        uint32_t len = strlen(pcString);
        uint32_t idx = 0;
        for (idx = 0; idx < len; idx++){
            putchar(*(pcString+idx));
        }
        /* The mutex MUST be given back! */
    }
    xSemaphoreGive( xMutex );
}

static void prvPrintTask( void *pvParameters )
{
    char *pcStringToPrint;
    const TickType_t xMaxBlockTimeTicks = 0x10;
    /* Two instances of this task are created. The string printed by the task is
    passed into the task using the task’s parameter. The parameter is cast to the
    required type. */
    pcStringToPrint = ( char * ) pvParameters;

    for( ;; )
    {
        /* Print out the string using the newly defined function. */
        prvNewPrintString( pcStringToPrint );
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
    printf("Mutex example\r\n");

    /* Before a semaphore is used it must be explicitly created. In this example a
    mutex type semaphore is created. */
    xMutex = xSemaphoreCreateMutex();
    /* Check the semaphore was created successfully before creating the tasks. */
    if (xMutex != NULL )
    {
        /* Create two instances of the tasks that write to stdout. The string they
        write is passed in to the task as the task’s parameter. The tasks are
        created at different priorities so some preemption will occur. */
        xTaskCreate( prvPrintTask, "Print1", configMINIMAL_STACK_SIZE,
        (char*)"Task 1 ***************************************\r\n", 1, NULL );

        xTaskCreate( prvPrintTask, "Print2", configMINIMAL_STACK_SIZE,
        (char*)"Task 2 ---------------------------------------\r\n", 2, NULL );

        /* Start the scheduler so the created tasks start executing. */
        vTaskStartScheduler();
    }

    /* As normal, the following line should never be reached. */
    for( ;; );
}