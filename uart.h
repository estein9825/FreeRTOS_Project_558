#ifndef UART_H
#define UART_H

#include <FreeRTOS.h>   // Include FreeRTOS headers for QueueHandle_t and other FreeRTOS types
#include <task.h>
#include <queue.h>

// Log queue
#define LOG_BUFFER_SIZE 256

// Declare uartQueue as an external variable
extern QueueHandle_t uartQueue;

// Declare task info queue for logging task-related information
extern QueueHandle_t logContextSwitchQueue;

// Declare function prototypes
void prvUARTInit(void);
void vLogContextSwitchTask(void *pvParameters);

#endif /* UART_H */
