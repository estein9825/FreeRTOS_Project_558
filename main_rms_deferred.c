#include "FreeRTOS.h"
#include <semphr.h>
#include "uart.h"
#include "trace_task_switch.h"
#include "tiny_print.h"
#include <task.h>

SemaphoreHandle_t xBinarySemaphore;

#define SIMPLE_LOW_DELAY                   100
#define SIMPLE_MEDIUM_DELAY                200
#define SIMPLE_HIGH_DELAY                  300
#define SIMPLE_DEFERRER_SERVER_DELAY       10

#define SIMPLE_LOW_COMPUTATION             2
#define SIMPLE_MEDIUM_COMPUTATION          3
#define SIMPLE_HIGH_COMPUTATION            5
#define SIMPLE_APERIODIC_COMPUTATION_MIN   1 // controls the maximum of the random range of computation
#define SIMPLE_APERIODIC_COMPUTATION_MAX   7 // controls the max of the random range of computation

#define APERIODIC_DELAY_MIN                30 // Minimum for delay range
#define APERIODIC_DELAY_MAX                100 // Maximum for delay range

#define SERVER_BUDGET_MS                50  // 50ms execution budget
#define SERVER_PERIOD_MS                100 // 100ms replenishment period


static TaskHandle_t xHighPriorityTask, xMediumPriorityTask, xLowPriorityTask, eventProducerHandle;

static unsigned long next = 1;

int rand(void)
{
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % 32768;
}

void srand(unsigned int seed)
{
    next = seed;
}

void runForTicks(int tickLength) {
    TickType_t startTick, currentTick;

    startTick = xTaskGetTickCount();
    do
    {
        // Get the current tick count
        currentTick = xTaskGetTickCount();
    } while ((currentTick - startTick) < tickLength); // Run for two ticks
}
void deferrableServerTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t lastWakeTime = xTaskGetTickCount();
    TickType_t remainingBudget = pdMS_TO_TICKS(SERVER_BUDGET_MS);
    TickType_t serverPeriod = pdMS_TO_TICKS(SERVER_PERIOD_MS);

    for (;;)
    {
        // Handle queued notifications
        while (ulTaskNotifyTake(pdTRUE, 0) > 0)
        {
            TickType_t interruptStartTime = xTaskGetTickCountFromISR();

            deferredServerActive = pdTRUE; // Mark the deferred server as active

            if (remainingBudget >= pdMS_TO_TICKS(SIMPLE_DEFERRER_SERVER_DELAY))
            {
                // Process the sporadic event
                vTaskDelay(pdMS_TO_TICKS(SIMPLE_DEFERRER_SERVER_DELAY));
                remainingBudget -= pdMS_TO_TICKS(SIMPLE_DEFERRER_SERVER_DELAY);
            }
            else
            {
                // Budget exhausted; skip event
                break;
            }
            deferredServerActive = pdFALSE; // Mark the deferred server as inactive

            TickType_t interruptEndTime = xTaskGetTickCountFromISR();
            deferredServerInterruptTime += (interruptEndTime - interruptStartTime);
            deferredServerInterruptCount++;
        }

        // Replenish the budget at the end of the period
        if ((xTaskGetTickCount() - lastWakeTime) >= serverPeriod)
        {
            TickType_t elapsedPeriods = (xTaskGetTickCount() - lastWakeTime) / serverPeriod;
            lastWakeTime += elapsedPeriods * serverPeriod;
            remainingBudget = pdMS_TO_TICKS(SERVER_BUDGET_MS);
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Avoid busy waiting
    }
}

void sporadicEventProducer(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        // Generate random delays and periods
        int sporadicDelay = APERIODIC_DELAY_MIN + rand() % (APERIODIC_DELAY_MAX - APERIODIC_DELAY_MIN + 1);
        int sporadicPeriod = SIMPLE_APERIODIC_COMPUTATION_MIN + 
                     (rand() % (SIMPLE_APERIODIC_COMPUTATION_MAX - SIMPLE_APERIODIC_COMPUTATION_MIN + 1));

        TickType_t interruptStartTime = xTaskGetTickCountFromISR();

        // Simulate sporadic event
        runForTicks(sporadicPeriod);
        vTaskDelay(pdMS_TO_TICKS(sporadicDelay));

        // Notify the deferrable server
        xTaskNotifyGive(serverTaskHandle);

        TickType_t interruptEndTime = xTaskGetTickCountFromISR();
        deferredServerInterruptTime += (interruptEndTime - interruptStartTime);
        deferredServerInterruptCount++;
    }
}


void lowTask(void *pvParameters)
{
    (void)pvParameters;
    // Task with lower priority
    for (;;)
    {
        if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdPASS)
        {
            runForTicks(SIMPLE_LOW_COMPUTATION);
            xSemaphoreGive(xBinarySemaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(SIMPLE_LOW_DELAY));
    }
}

void mediumTask(void *pvParameters)
{
    (void)pvParameters;
    // Task with medium priority
    for (;;)
    {
        if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdPASS)
        {
            runForTicks(SIMPLE_MEDIUM_COMPUTATION);
            xSemaphoreGive(xBinarySemaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(SIMPLE_MEDIUM_DELAY));
    }
}

void highTask(void *pvParameters)
{
    (void)pvParameters;
    // Task with high priority
    for (;;)
    {
        if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdPASS)
        {           
            runForTicks(SIMPLE_HIGH_COMPUTATION);
            xSemaphoreGive(xBinarySemaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(SIMPLE_HIGH_DELAY));
    }
}

void main_rms_deferred(void)
{
    initializeTaskTracking();
    xBinarySemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(xBinarySemaphore);
    
    const char taskNameLow[] = "Low";
    const char taskNameMed[] = "Med";
    const char taskNameHigh[] = "High";
    const char taskAperiodic[] = "Aperiodic";

    if (xTaskCreate(lowTask, taskNameLow, configMINIMAL_STACK_SIZE, NULL, SIMPLE_LOW_PRIORITY, &xLowPriorityTask) == pdPASS) {
        setTaskNameFromISR(xLowPriorityTask, taskNameLow, 1);
    }
    if (xTaskCreate(mediumTask, taskNameMed, configMINIMAL_STACK_SIZE*2, NULL, SIMPLE_MEDIUM_PRIROITY, &xMediumPriorityTask) == pdPASS) {
        setTaskNameFromISR(xMediumPriorityTask, taskNameMed, 2);
    }
    if (xTaskCreate(highTask, taskNameHigh, configMINIMAL_STACK_SIZE*4, NULL, SIMPLE_HIGH_PRIORITY, &xHighPriorityTask) == pdPASS) {
        setTaskNameFromISR(xHighPriorityTask, taskNameHigh, 3);
    }

    xTaskCreate(deferrableServerTask, "DeferrableServer", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+2, &serverTaskHandle);
    if (xTaskCreate(sporadicEventProducer, taskAperiodic, configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, &eventProducerHandle) == pdPASS) {
        setTaskNameFromISR(eventProducerHandle, taskAperiodic, 4);
    }

    xTaskCreate(vLogContextSwitchTask, "RMS Log Switch Task", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY, NULL);

    vTaskStartScheduler();

}
