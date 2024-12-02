#include "trace_task_switch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "tiny_print.h"
#include <string.h>
#include "timers.h"

// Global arrays for storing task information
TaskInfo taskInfo[MAX_TASKS];  // Store task details like name, state, ID, etc.
TaskHandle_t taskHandles[MAX_TASKS];  // Store task handles for reference

// used to track server handle interrupts
TaskHandle_t serverTaskHandle;

// Global variables for latency overhead tracking
volatile static TickType_t totalSystemTime = 0;
volatile static TickType_t totalTaskExecutionTime = 0;
volatile static TickType_t totalContextSwitchTime = 0;
volatile static TickType_t totalInterruptTime = 0;

volatile TickType_t deferredServerInterruptTime = 0;
volatile uint32_t deferredServerInterruptCount = 0;
volatile BaseType_t deferredServerActive = pdFALSE;

// Global counters
volatile uint32_t highPriorityTaskCount = 0;
volatile uint32_t mediumPriorityTaskCount = 0;
volatile uint32_t lowPriorityTaskCount = 0;
volatile uint32_t aperiodicTaskCount = 0;


// Function to count tasks
void classifyAndCountTask(UBaseType_t taskPriority) {

    if (taskPriority == SIMPLE_HIGH_PRIORITY) {
        highPriorityTaskCount++;
    } else if (taskPriority == SIMPLE_MEDIUM_PRIROITY) {
        mediumPriorityTaskCount++;
    } else if (taskPriority == SIMPLE_LOW_PRIORITY) {
        lowPriorityTaskCount++;
    } else if (taskPriority == SIMPLE_APERIODIC_PRIORTY) {
        aperiodicTaskCount++;
    }
}

// function to print latency overheads
void printLatencyOverhead(void) {
    totalSystemTime = xTaskGetTickCount();

    float overhead = (float)(totalContextSwitchTime + totalInterruptTime) / totalSystemTime;
    printf("\n");
    printf("==== Latency Overhead Report ====\n");
    printf("Total System Time: %lu ticks\n", totalSystemTime);
    printf("Task Execution Time: %lu ticks\n", totalTaskExecutionTime);
    printf("Context Switch Time: %lu ticks\n", totalContextSwitchTime);
    printf("Interrupt Time: %lu ticks\n", totalInterruptTime);
    printf("Latency Overhead: %.2f%%\n", overhead);
}

// Function to print aperiodic interrupt contributions
void printAperiodicInterruptContribution(void)
{
    float aperiodicInterruptPercentage = (float)deferredServerInterruptTime / totalInterruptTime * 100;

    printf("\n==== Aperiodic Interrupt Contribution ====\n");
    printf("Total Interrupt Time: %lu ticks\n", totalInterruptTime);
    printf("Aperiodic Interrupt Time: %lu ticks\n", deferredServerInterruptTime);
    printf("Deferred Server Interrupt Count: %lu\n", deferredServerInterruptCount);
    printf("Aperiodic Interrupt Contribution: %.2f%%\n", aperiodicInterruptPercentage);
}

// Function to print the task counts
void printTaskCounts(void) {
    printf("\n========= Task Counts =========\n");
    printf("High Priority Tasks: %u\n", highPriorityTaskCount);
    printf("Medium Priority Tasks: %u\n", mediumPriorityTaskCount);
    printf("Low Priority Tasks: %u\n", lowPriorityTaskCount);
    printf("Aperiodic Tasks: %u\n", aperiodicTaskCount);
}

// Function to set task name and other info from ISR (interrupt context)
void setTaskNameFromISR(TaskHandle_t xTaskHandle, const char *taskName, int taskId) {
    if (taskId < MAX_TASKS && taskName != NULL && xTaskHandle != NULL) {

        strncpy(taskInfo[taskId].taskName, taskName, MAX_TASK_NAME_LENGTH - 1);
        taskInfo[taskId].taskName[MAX_TASK_NAME_LENGTH - 1] = '\0'; // Null-terminate
        taskHandles[taskId] = xTaskHandle;

        // printf("Registered Task[%d]: Handle=%p, Name=%s\n", taskId, (void *)xTaskHandle, taskName);

    } else {
        printf("Task registration failed: ID=%d, Name='%s', Handle=%p\n",
               taskId, taskName ? taskName : "(null)", (void *)xTaskHandle);
        configASSERT(0); // Catch invalid registration
    }
}

void initializeTaskTracking(void) {
    for (UBaseType_t i = 0; i < MAX_TASKS; ++i) {
        taskHandles[i] = NULL;
        memset(taskInfo[i].taskName, 0, MAX_TASK_NAME_LENGTH);  // Clear task name
        taskInfo[i].taskId = -1;         // Invalid task ID
        taskInfo[i].lastSwitchIn = 0;
        taskInfo[i].state = eSuspended; // Default state
    }
    // printf("Task tracking initialized.\n");
}

// Trace function called when a task is switched in
void traceTaskSwitchedIn(void) {
    TaskHandle_t xTaskHandle = xTaskGetCurrentTaskHandle();
    TickType_t taskSwitchInTime = xTaskGetTickCountFromISR();

    if (xTaskHandle != NULL) {

        if (xTaskHandle == xTimerGetTimerDaemonTaskHandle()) {
            // Skip tracking for the timer task
            return;
        }

        // Find the task index corresponding to the current task handle
        UBaseType_t taskIndex = MAX_TASKS;  // Default to invalid index

        for (UBaseType_t i = 0; i < MAX_TASKS; ++i) {
            if (taskHandles[i] == xTaskHandle) {
                taskIndex = i;
                break;
            }
        }

        // If a valid task index was found, proceed with logging
        if (taskIndex < MAX_TASKS) {
            // Update total task execution time before resetting lastSwitchIn
            if (taskInfo[taskIndex].lastSwitchIn != 0) { // Ensure it's not the first switch-in
                totalTaskExecutionTime += taskSwitchInTime - taskInfo[taskIndex].lastSwitchIn;
            }

            taskInfo[taskIndex].lastSwitchIn = taskSwitchInTime; // Update last switch-in time
         }
    }
}

// Trace function called when a task is switched out
void traceTaskSwitchedOut(void) {
    TaskHandle_t xTaskHandle = xTaskGetCurrentTaskHandle();
    TickType_t taskSwitchOutTime = xTaskGetTickCountFromISR();


    if (xTaskHandle != NULL) {

        if (xTaskHandle == xTimerGetTimerDaemonTaskHandle()) {
            // Skip tracking for the timer task
            return;
        }
        // Find the task index corresponding to the current task handle
        UBaseType_t taskIndex = MAX_TASKS;  // Default to invalid index

        for (UBaseType_t i = 0; i < MAX_TASKS; ++i) {
            if (taskHandles[i] == xTaskHandle) {
                taskIndex = i;
                break;
            }
        }

        // If a valid task index was found, proceed with logging
        if (taskIndex < MAX_TASKS) {
            // Calculate latency (time spent in task)
            TickType_t timeSpentInTask = taskSwitchOutTime - taskInfo[taskIndex].lastSwitchIn;
            totalContextSwitchTime += timeSpentInTask; // Approximate context switch time
            taskInfo[taskIndex].state = eBlocked;  // Assuming the task is blocked after switching out

            UBaseType_t taskPriority = uxTaskPriorityGetFromISR(xTaskHandle);
            classifyAndCountTask(taskPriority);

            // Create a log message for task switched out with latency info
            LogMessage logMessage = {
                .taskId = taskIndex,
                .priority = taskPriority,
                .timestamp_in = taskInfo[taskIndex].lastSwitchIn,
                .timestamp_out = taskSwitchOutTime,
                .timeSpentInTask = timeSpentInTask,  // Store time spent in the task
                .message = "out"
            };

            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xQueueSendToBackFromISR(logContextSwitchQueue, &logMessage, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        // } else {
        //     printf("Ignorining task: %s, handle: %p\n", pcTaskGetName(xTaskHandle), (void *)xTaskHandle);
        }
    }
}

void myTraceISR_ENTER(void)
{
    TickType_t startInterruptTime = xTaskGetTickCountFromISR();
    totalInterruptTime -= startInterruptTime;

    // Check if the deferred server is running
    if (deferredServerActive)
    {
        deferredServerInterruptTime -= startInterruptTime;
    }
}

void myTraceISR_EXIT(void)
{
    TickType_t endInterruptTime = xTaskGetTickCountFromISR();
    totalInterruptTime += endInterruptTime;

    // Check if the deferred server is running
    if (deferredServerActive)
    {
        deferredServerInterruptTime += endInterruptTime;
    }
}
