#ifndef CYCLIC_EXECUTIVE_H 
#define CYCLIC_EXECUTIVE_H 

#include "task_config.h" 

/**
 * status codes
 */


typedef enum
{
    CE_SUCCESS,
    CE_ERROR_INVALID_ARGUMENT,
    CE_ERROR_MAX_TASKS_REACHED,
    CE_ERROR_NOT_INITIALIZED,
    CE_ERROR_UNSCHEDULABLE

} CE_Status;

/* Initialize the cyclic executive platform */

CE_Status CE_Init(void);

/* register a user-defined periodic task */

CE_Status CE_RegisterTask(
    const char *task_name, 
    CE_TaskFunction task_function, 
    uint32_t period_ms, 
    uint32_t wcet_ms
);

/* get the number of registered tasks */

uint32_t CE_GetTaskCount(void);

/* Access registered task information */

const CE_Task *CE_GetTask(uint32_t index);

/*
 * Build the cyclic schedule.
 * Computes the major frame, minor frame, and the task list for every frame.
 */
CE_Status CE_BuildSchedule(void);

/* Print the frame size and the task-to-frame table. */
void CE_PrintSchedule(void);

uint32_t CE_GetMajorFrame(void);
uint32_t CE_GetMinorFrame(void);
uint32_t CE_GetFrameCount(void);

/*
 * Run the schedule on FreeRTOS.
 * major_cycles is how many hyperperiods to execute.
 * Pass 0 to run until the process is stopped.
 * Returns after the scheduler stops.
 */
CE_Status CE_Start(uint32_t major_cycles);

#endif