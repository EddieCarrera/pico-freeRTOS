#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// #define UNIQUE_CALLBACK_EXAMPLE
#define SINGLE_CALLBACK_TIMERID_EXAMPLE

/***************************** Important Notes *****************************
 * 1) By default, the max timers available in the deamon queue is set by 
 * configTIMER_QUEUE_LENGTH. The default priority is configMAX_PRIORITIES-1. 
 * 
 * 2) The pvTimerID field can be used by the application writer for any 
 * purpose. It seems like a callback would be the most practical application.
 * The same callback function can be assigned to more than one software timer. 
 * When that is done, the callback function parameter is used to determine 
 * which software timer expired.
 * 
 * 3) Unlike other software timer API functions, vTimerSetTimerID() and 
 * pvTimerGetTimerID() access the software timer directly—they do not send 
 * a command to the timer command queue.
 * 
 * 4) If xTimerChangePeriod() is used to change the period of a timer that 
 * is already running, then the timer will use the new period value to 
 * recalculate its expiry time. The recalculated expiry time is relative to 
 * when xTimerChangePeriod() was called, not relative to when the timer was 
 * originally started. 
 * 
 * 5) If xTimerChangePeriod() is used to change the period of a timer that 
 * is in the Dormant state (a timer that is not running), then the timer 
 * will calculate an expiry time, and transition to the Running state 
 * (the timer will start running).
*****************************************************************************/

#define main_ONESHOT_TIMER_PERIOD   pdMS_TO_TICKS(3333)
#define main_RELOAD_TIMER_PERIOD    pdMS_TO_TICKS(500)

TimerHandle_t autoReload_timer, oneShot_timer;
BaseType_t timer1Started, timer2Started;

/* prvTimerCallback() will execute when either timer expires. 
 * The implementation of prvTimerCallback() uses the function’s parameter to 
 * determine if it was called because the one-shot timer expired, or 
 * because the auto-reload timer expired. prvTimerCallback() also 
 * demonstrates how to use the software timer ID as timer specific storage; 
 * each software timer keeps a count of the number of times it has expired 
 * in its own ID, and the auto-reload timer uses the count to stop itself 
 * the fifth time it executes. */
#ifdef SINGLE_CALLBACK_TIMERID_EXAMPLE
void prvTimerCallback(TimerHandle_t currTimer)
{
    TickType_t timeNow;
    uint32_t ctr;

    /* A count of the number of times this software timer has expired is 
    stored in the timer's ID. Obtain the ID, increment it, then save it as
    the new ID value. The ID is a void pointer, so is cast to a uint32_t. */
    ctr = (uint32_t)pvTimerGetTimerID(currTimer);
    vTimerSetTimerID(currTimer, (void*)(++ctr));

    timeNow = xTaskGetTickCount();

    if (currTimer == oneShot_timer)
    {
        printf("One-shot timer callback executing: %u\n", timeNow);
    }
    else {
        printf("Auto-reload timer callback executing: %u\n", timeNow);

        if (ctr++ == 5)
        {
            /* This callback function executes in the context of the RTOS 
               daemon task so must not call any functions that might place 
               the daemon task into the Blocked state. Therefore a block time 
               of 0 is used. */
            xTimerStop(currTimer, 0);
        }
    }
}
#endif
#ifdef UNIQUE_CALLBACK_EXAMPLE
void pvOneShotTimerCallback(TimerHandle_t xTimer)
{
    TickType_t timeNow;
    timeNow = xTaskGetTickCount();
    printf("One-shot timer callback executing: %u\n", timeNow);
}

void pvAutoReloadTimerCallback(TimerHandle_t xTimer)
{
    TickType_t timeNow;
    timeNow = xTaskGetTickCount();
    printf("Auto-reload timer callback executing: %u\n", timeNow);
}
#endif

int main (void)
{
    stdio_init_all();
    while(!stdio_usb_connected()){tight_loop_contents();}

#ifdef SINGLE_CALLBACK_TIMERID_EXAMPLE
    oneShot_timer = xTimerCreate(
        "OneShot",
        main_ONESHOT_TIMER_PERIOD,
        pdFALSE,
        0,  // Initialize timerID as 0
        prvTimerCallback
    );

    autoReload_timer = xTimerCreate(
        "autoReload",
        main_RELOAD_TIMER_PERIOD,
        pdTRUE,
        0,  // Initialize timerID as 0
        prvTimerCallback
    );   
#endif

#ifdef UNIQUE_CALLBACK_EXAMPLE
    oneShot_timer = xTimerCreate(
        "OneShot",
        main_ONESHOT_TIMER_PERIOD,
        pdFALSE,
        NULL,
        pvOneShotTimerCallback
    );

    autoReload_timer = xTimerCreate(
        "autoReload",
        main_RELOAD_TIMER_PERIOD,
        pdTRUE,
        NULL,
        pvAutoReloadTimerCallback
    );    
#endif
    /* Check the software timers were created. */
    if( ( oneShot_timer != NULL ) && ( autoReload_timer != NULL ) ) 
    {
        /* Start the software timers, using a block time of 0 (no block time). 
        The scheduler has not been started yet so any block time specified 
        here would be ignored anyway. */
        timer1Started = xTimerStart(oneShot_timer, 0);
        timer2Started = xTimerStart(autoReload_timer, 0);

        if ( timer1Started == pdPASS && timer2Started == pdPASS )
        {
            vTaskStartScheduler();
        }

    }
    /* As always, this line should not be reached. */
    for( ;; );
}