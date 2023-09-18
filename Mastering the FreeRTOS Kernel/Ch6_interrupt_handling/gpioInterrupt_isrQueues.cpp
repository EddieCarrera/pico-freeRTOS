#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <semphr.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *********************************
 * 1) Binary and counting semaphores are used to communicate events. Queues are 
 * used to communicate events, and to transfer data.
 * 
 * 2) Considerations When Using a Queue From an ISR
 * Queues provide an easy and convenient way of passing data from an interrupt 
 * to a task, but it is not efficient to use a queue if data is arriving at a high 
 * frequency. Unless the data is arriving slowing, it is recommended that 
 * production code does not copy the technique. More efficient techniques, that 
 * are suitable for production code, include: 
 * 
 * Using Direct Memory Access (DMA) hardware to receive and buffer characters. 
 * This method has practically no software overhead. A direct to task notification 
 * can then be used to unblock the task that will process the buffer only after a 
 * break in transmission has been detected.
 * 
 * NOTE: Direct to task notifications provide the most efficient method of unblocking 
 * a task from an ISR. Direct to task notifications are covered in Chapter 9.
 * 
 * Copying each received character into a thread safe RAM buffer. Again, a direct 
 * to task notification can be used to unblock the task that will process the 
 * buffer after a complete message has been received, or after a break in transmission 
 * has been detected.
 * 
 * NOTE: The ‘Stream Buffer’, provided as part of FreeRTOS+TCP 
 * (http://www.FreeRTOS.org/tcp), can be used for this purpose.
 * 
 * Processing the received characters directly within the ISR, then using a queue 
 * to send just the result of processing the data (rather than the raw data) to a 
 * task. 
 *******************************************************************************/