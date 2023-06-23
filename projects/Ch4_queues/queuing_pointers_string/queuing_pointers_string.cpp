#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

/***************************** Important Notes *****************************
 * 1) Notice how queue functions will always work with the address of the
 *    data being worked with. This is a gotcha, as not referencing '&'
 *    correctly will cause the code to not work. 
 * 
 *    In this case, notice how strings to queues are sent by sending
 *    the address of the pointer that points to the buffer
 *    a.k.a a pointer to a pointer
*****************************************************************************/

QueueHandle_t xPointerQueue;

void sendingTask(void *pvParameter)
{
    char *pcStringToSend;
    const size_t xMaxStringLength = 50;
    BaseType_t xStringNumber = 0;
    while(1)
    {
        pcStringToSend = (char*)pvPortMalloc(sizeof(char)*xMaxStringLength);
        sprintf(pcStringToSend, "Sending #%d message via pointer\r\n", xStringNumber);
        xStringNumber++;
        /* Send the address of the pointer that points to the buffer */
        xQueueSend(xPointerQueue, &pcStringToSend, 0); 
    }

}

void receiveTask(void *pvParameter)
{
    char *pvReceivedString = NULL;
    while(1)
    {
        /* Store the buffer’s address in pcReceivedString. */
        xQueueReceive(xPointerQueue, &pvReceivedString, portMAX_DELAY);
        printf("%s", pvReceivedString);
        vPortFree(pvReceivedString); // Dont forget to free!
    }
}

int main(void)
{
    xPointerQueue = xQueueCreate(5, sizeof(char *));
    stdio_init_all();

    xTaskCreate(sendingTask, "Transmit1", configMINIMAL_STACK_SIZE, 
                NULL, 1, NULL);
    xTaskCreate(receiveTask, "ReceiveTask", configMINIMAL_STACK_SIZE, 
                NULL, 2, NULL);

    vTaskStartScheduler();

 /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks.  If main() does reach here then it is likely that
    there was insufficient heap memory available for the idle task to be created */
    printf("ERROR\r\n");
    while(1);
}

