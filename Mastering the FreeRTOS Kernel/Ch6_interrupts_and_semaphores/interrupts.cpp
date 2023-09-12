#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *****************************
 * 1) FreeRTOS API functions perform actions that are not valid inside an 
 * ISR—the most notable of which is placing the task that called the API 
 * function into the Blocked state. Never call a FreeRTOS API function that 
 * does not have “FromISR” in its name from an ISR.
 * 
 * 2) If a context switch is performed by an interrupt, then the task running 
 * when the interrupt exits might be different to the task that was running 
 * when the interrupt was entered—the interrupt will have interrupted one task, 
 * but returned to a different task. 
 * 
 * A switch to a higher priority task will not occur automatically inside an 
 * interrupt. Instead, a variable is set to inform the application writer that 
 * a context switch should be performed. Interrupt safe API functions (those 
 * that end in “FromISR”) have a pointer parameter called pxHigherPriorityTaskWoken 
 * that is used for this purpose.
 * 
 * If the application writer opts not to request a context switch from the ISR, 
 * then the higher priority task will remain in the Ready state until the next 
 * time the scheduler runs—which in the worst case will be during the next tick 
 * interrupt.
 * 
*****************************************************************************/
