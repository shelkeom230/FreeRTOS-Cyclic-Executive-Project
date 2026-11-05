#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "cyclic_executive.h"
#include "user_tasks.h"

/* How many hyperperiods the demo runs before it stops and prints the check. */
#define CE_DEMO_MAJOR_CYCLES 2

/* Static memory for Idle Task */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];


/* Static memory for Timer Task */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];


/* Required when static allocation is enabled */
void vApplicationGetIdleTaskMemory(
    StaticTask_t **ppxIdleTaskTCBBuffer,
    StackType_t **ppxIdleTaskStackBuffer,
    configSTACK_DEPTH_TYPE *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}


void vApplicationGetTimerTaskMemory(
    StaticTask_t **ppxTimerTaskTCBBuffer,
    StackType_t **ppxTimerTaskStackBuffer,
    configSTACK_DEPTH_TYPE *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}


/* Required application hooks */
void vApplicationTickHook(void)
{
}


void vApplicationIdleHook(void)
{
}


void vApplicationDaemonTaskStartupHook(void)
{
}


void vApplicationMallocFailedHook(void)
{
    printf("FreeRTOS memory allocation failed!\n");
    exit(1);
}

void vLoggingPrintf(const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    vprintf(format, arguments);
    va_end(arguments);
}


/* Required by this FreeRTOS POSIX simulator configuration */
void vAssertCalled(const char *file, unsigned long line)
{
    printf("FreeRTOS assertion failed: %s:%lu\n", file, line);
    exit(1);
}

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);

    printf("\n");
    printf("============================================\n");
    printf("       CYCLIC EXECUTIVE ON FREERTOS\n");
    printf("============================================\n");


    /* Initialize the platform */

    if (CE_Init() != CE_SUCCESS)
    {
        printf("Initialization failed!\n");
        return 1;
    }


    /* Register user-defined tasks */

    CE_RegisterTask("Sensor", read_sensor, 100, 10);

    CE_RegisterTask("Display", update_display, 200, 20);

    CE_RegisterTask("Temperature", calculate_temperature, 500, 30);

    CE_RegisterTask("Data", send_data, 1000, 40);


    /* Display registered task information */

    printf("\nRegistered Tasks:\n");

    printf("--------------------------------------------------\n");

    printf("%-15s %-15s %-15s\n", "Task", "Period(ms)", "WCET(ms)");

    printf("--------------------------------------------------\n");


    for (uint32_t i = 0; i < CE_GetTaskCount(); i++)
    {
        const CE_Task *task = CE_GetTask(i);

        printf("%-15s %-15u %-15u\n",
               task->task_name,
               (unsigned int) task->period_ms,
               (unsigned int) task->wcet_ms);
    }


    printf("--------------------------------------------------\n");

    printf("\nTotal registered tasks: %u\n",
           (unsigned int) CE_GetTaskCount());

    if (CE_BuildSchedule() != CE_SUCCESS)
    {
        printf("Schedule construction failed. The task set does not fit the cyclic executive.\n");
        return 1;
    }

    CE_PrintSchedule();

    if (CE_Start(CE_DEMO_MAJOR_CYCLES) != CE_SUCCESS)
    {
        printf("Cyclic executive failed to start.\n");
        return 1;
    }

    return 0;
}