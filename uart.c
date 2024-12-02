#include "uart.h"  // Include the header file for uartQueue and function declarations
#include "queue.h"
#include <string.h>
#include "FreeRTOSConfig.h"
#include "trace_task_switch.h"
#include "tiny_print.h"

QueueHandle_t logContextSwitchQueue = NULL;  // Define the logContextSwitchQueue handle

/* printf() output uses the UART.  These constants define the addresses of the
 * required UART registers. */
#define UART0_ADDRESS                         ( 0x40004000UL )
#define UART0_CTRL                            ( *( ( ( volatile uint32_t * ) ( UART0_ADDRESS + 8UL ) ) ) )
#define UART0_BAUDDIV                         ( *( ( ( volatile uint32_t * ) ( UART0_ADDRESS + 16UL ) ) ) )
#define MAX_LOG_MESSAGES 50

/* UART initialization function */
void prvUARTInit(void)
{
    logContextSwitchQueue = xQueueCreate(MAX_LOG_MESSAGES, sizeof(LogMessage));  // Create the log queue
    if (logContextSwitchQueue == NULL) {
        // Handle error: Queue creation failed
        printf("Log Queue creation failed! Heap used: %lu bytes\n", configTOTAL_HEAP_SIZE - xPortGetFreeHeapSize());
        while (1);  // Infinite loop to halt execution
    }

    // Other UART initialization (e.g., baud rate, control registers)
    UART0_BAUDDIV = 5207;
    UART0_CTRL = 0x7;
}

// Log messages from context switching
void vLogContextSwitchTask(void *pvParameters)
{
    (void)pvParameters;
    printf("Task Name,Priority,Switched In (ticks), Switched Out (ticks), Spent In task(ticks)\n");
    LogMessage logMessage;
    while (1)
    {
        if (xQueueReceive(logContextSwitchQueue, &logMessage, portMAX_DELAY) == pdTRUE)
        {
            // Format and print the task context switch information (including latency)
            printf("\"%s\",%lu,%lu,%lu,%lu\n", 
                   taskInfo[logMessage.taskId].taskName,
                   logMessage.priority,
                   logMessage.timestamp_in,
                   logMessage.timestamp_out,
                   logMessage.timeSpentInTask);
        }
        else {
            printf("Error receiving from Log Queue\n");
        }
    }
}
