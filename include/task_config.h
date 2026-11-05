#ifndef TASK_CONFIG_H
#define TASK_CONFIG_H

#include <stdint.h>

/**
 * max number of user tasks supported
 */
#define CE_MAX_TASKS 20 

/**
 * func ptr type 
 */

typedef void (*CE_TaskFunction)(void);

/**
 * structure representing a user task
 */

typedef struct {
    const char *task_name;

    CE_TaskFunction task_function;

    uint32_t period_ms;

    uint32_t wcet_ms;
} CE_Task;

#endif 

