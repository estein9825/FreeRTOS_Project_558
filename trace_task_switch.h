#ifndef TRACE_TASK_SWITCH_H
#define TRACE_TASK_SWITCH_H

#include "portmacro.h"  // Required for FreeRTOS types like BaseType_t, TickType_t, etc.

#define MAX_TASKS 10
#define MAX_TASK_NAME_LENGTH 100  // Define a reasonable maximum length

#define SIMPLE_LOW_PRIORITY                ( tskIDLE_PRIORITY + 3 )
#define SIMPLE_MEDIUM_PRIROITY             ( tskIDLE_PRIORITY + 4 )
#define SIMPLE_HIGH_PRIORITY               ( tskIDLE_PRIORITY + 5 )
#define SIMPLE_APERIODIC_PRIORTY           ( tskIDLE_PRIORITY + 1 )

// Forward declarations of FreeRTOS types
typedef struct tskTaskControlBlock * TaskHandle_t;  // TaskHandle_t is a pointer to tskTaskControlBlock
typedef struct QueueDefinition * QueueHandle_t;    // QueueHandle_t is a pointer to QueueDefinition

// Define a structure to hold task information
typedef struct {
    char taskName[MAX_TASK_NAME_LENGTH];
    int taskId;
    TickType_t lastSwitchIn;
    UBaseType_t state;  // Example: store the task state
    TickType_t stackHighWaterMark; // Store stack high watermark
} TaskInfo;

// Define a structure to store log message details
typedef struct {
    int taskId;
    UBaseType_t priority;
    TickType_t timestamp_in;
    TickType_t timestamp_out;
    TickType_t timeSpentInTask;
    char message[80];  // Fixed message length (or dynamically allocated if needed)
    
} LogMessage;

// Declare the logContextSwitchQueue handle (extern, it will be defined elsewhere)
extern QueueHandle_t logContextSwitchQueue;

// Declare the task-related functions (we'll define them in trace_task_switch.c)
void setTaskNameFromISR(TaskHandle_t xTaskHandle, const char *taskName, int taskId);
void initializeTaskTracking(void);
void printLatencyOverhead(void);
void printTaskCounts(void);

// Declare an array to store task information (for latency and other data)
extern TaskInfo taskInfo[MAX_TASKS];
extern TaskHandle_t taskHandles[MAX_TASKS];  // Array to store task handles

// Macros for task switching trace functions
#define traceTASK_SWITCHED_IN()  traceTaskSwitchedIn()
#define traceTASK_SWITCHED_OUT() traceTaskSwitchedOut()
#define traceISR_ENTER()         myTraceISR_ENTER()
#define traceISR_EXIT()          myTraceISR_EXIT()

extern TaskHandle_t serverTaskHandle;
extern volatile TickType_t deferredServerInterruptTime;
extern volatile uint32_t deferredServerInterruptCount;
extern volatile BaseType_t deferredServerActive;

#endif /* TRACE_TASK_SWITCH_H */
