#include <stdio.h>

#include "cyclic_executive.h"

/* Internal task registry */

static CE_Task task_registry[CE_MAX_TASKS];

/* Number of currently registered tasks */

static uint32_t registered_task_count = 0;

/* Initialization status */

static int initialized = 0;


/* Initialize Cyclic Executive */

CE_Status CE_Init(void)
{
    registered_task_count = 0;

    initialized = 1;

    printf("\nCyclic Executive initialized successfully.\n");

    return CE_SUCCESS;
}


/* Register a new user task */

CE_Status CE_RegisterTask(
    const char *task_name,
    CE_TaskFunction task_function,
    uint32_t period_ms,
    uint32_t wcet_ms
)
{
    if (!initialized)
    {
        return CE_ERROR_NOT_INITIALIZED;
    }

    /* Validate task information */

    if (task_name == NULL ||
        task_function == NULL ||
        period_ms == 0 ||
        wcet_ms == 0 ||
        wcet_ms > period_ms)
    {
        return CE_ERROR_INVALID_ARGUMENT;
    }

    /* Check registry capacity */

    if (registered_task_count >= CE_MAX_TASKS)
    {
        return CE_ERROR_MAX_TASKS_REACHED;
    }

    /* Store task information */

    task_registry[registered_task_count].task_name = task_name;

    task_registry[registered_task_count].task_function = task_function;

    task_registry[registered_task_count].period_ms = period_ms;

    task_registry[registered_task_count].wcet_ms = wcet_ms;

    registered_task_count++;

    return CE_SUCCESS;
}


/* Return number of registered tasks */

uint32_t CE_GetTaskCount(void)
{
    return registered_task_count;
}


/* Return task information using index */

const CE_Task *CE_GetTask(uint32_t index)
{
    if (index >= registered_task_count)
    {
        return NULL;
    }

    return &task_registry[index];
}