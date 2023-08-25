#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *****************************
 * 1) By default, the max timers available in the deamon queue is set by 
 * configTIMER_QUEUE_LENGTH. The default priority is configMAX_PRIORITIES-1. 
*****************************************************************************/

#define main_ONESHOT_TIMER_PERIOD   pdMS_TO_TICKS(3333)
#define main_RELOAD_TIMER_PERIOD    pdMS_TO_TICKS(500)

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

int main (void)
{
    TimerHandle_t autoReload_timer, oneShot_timer;
    BaseType_t timer1Started, timer2Started;

    stdio_init_all();
    while(!stdio_usb_connected()){tight_loop_contents();}

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