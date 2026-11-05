#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "cyclic_executive.h"
#include "frame_calculator.h"

/*
 * Upper bound on minor frames inside one major frame.
 * The demo task set produces 25 frames; this leaves room for other periods.
 */
#define CE_MAX_FRAMES 256

/* Dispatcher runs above the idle task and below the timer service task. */
#define CE_DISPATCHER_PRIORITY (tskIDLE_PRIORITY + 2)

static uint32_t major_frame_ms;
static uint32_t minor_frame_ms;
static uint32_t frame_count;
static int schedule_ready;

/* task_slots[frame][n] holds the registry index of the nth job in that frame. */
static uint32_t task_slots[CE_MAX_FRAMES][CE_MAX_TASKS];
static uint32_t slot_count[CE_MAX_FRAMES];

static uint32_t jobs_executed[CE_MAX_TASKS];
static uint32_t configured_major_cycles;

static uint32_t ceil_div(uint32_t numerator, uint32_t denominator)
{
    return (numerator + denominator - 1U) / denominator;
}

static uint32_t frame_load_ms(uint32_t frame)
{
    uint32_t load = 0;

    for (uint32_t slot = 0; slot < slot_count[frame]; slot++)
    {
        const CE_Task *task = CE_GetTask(task_slots[frame][slot]);
        load += task->wcet_ms;
    }

    return load;
}

static void clear_schedule(void)
{
    schedule_ready = 0;
    major_frame_ms = 0;
    minor_frame_ms = 0;
    frame_count = 0;

    for (uint32_t frame = 0; frame < CE_MAX_FRAMES; frame++)
    {
        slot_count[frame] = 0;
    }
}

/*
 * Place every job of one task into a frame that
 *   - starts at or after the job release, and
 *   - ends at or before the job deadline (deadline = period).
 * The earliest frame that still has room for the WCET is used.
 */
static CE_Status assign_task(uint32_t task_index)
{
    const CE_Task *task = CE_GetTask(task_index);
    uint32_t jobs = major_frame_ms / task->period_ms;

    for (uint32_t job = 0; job < jobs; job++)
    {
        uint32_t release = job * task->period_ms;
        uint32_t deadline = release + task->period_ms;
        uint32_t first_frame = ceil_div(release, minor_frame_ms);
        uint32_t last_frame;
        int placed = 0;

        if (deadline < minor_frame_ms)
        {
            return CE_ERROR_UNSCHEDULABLE;
        }

        last_frame = (deadline / minor_frame_ms) - 1U;

        if (first_frame > last_frame || last_frame >= frame_count)
        {
            return CE_ERROR_UNSCHEDULABLE;
        }

        for (uint32_t frame = first_frame; frame <= last_frame; frame++)
        {
            if (slot_count[frame] >= CE_MAX_TASKS)
            {
                continue;
            }

            if (frame_load_ms(frame) + task->wcet_ms > minor_frame_ms)
            {
                continue;
            }

            task_slots[frame][slot_count[frame]] = task_index;
            slot_count[frame]++;
            placed = 1;
            break;
        }

        if (!placed)
        {
            return CE_ERROR_UNSCHEDULABLE;
        }
    }

    return CE_SUCCESS;
}

CE_Status CE_BuildSchedule(void)
{
    CE_Status status;

    clear_schedule();

    status = CE_CalculateFrames(&major_frame_ms, &minor_frame_ms, &frame_count);
    if (status != CE_SUCCESS)
    {
        return status;
    }

    if (frame_count == 0 || frame_count > CE_MAX_FRAMES || minor_frame_ms == 0)
    {
        clear_schedule();
        return CE_ERROR_UNSCHEDULABLE;
    }

    for (uint32_t index = 0; index < CE_GetTaskCount(); index++)
    {
        status = assign_task(index);
        if (status != CE_SUCCESS)
        {
            clear_schedule();
            return status;
        }
    }

    schedule_ready = 1;
    return CE_SUCCESS;
}

void CE_PrintSchedule(void)
{
    if (!schedule_ready)
    {
        printf("No schedule has been built.\n");
        return;
    }

    printf("\n========== FRAME CALCULATION ==========\n");
    printf("Major frame : %u ms\n", (unsigned int) major_frame_ms);
    printf("Minor frame : %u ms\n", (unsigned int) minor_frame_ms);
    printf("Frame count : %u\n", (unsigned int) frame_count);
    printf("=======================================\n");

    printf("\n========== TASK TO FRAME ASSIGNMENT ==========\n");

    for (uint32_t frame = 0; frame < frame_count; frame++)
    {
        uint32_t start = frame * minor_frame_ms;
        uint32_t end = start + minor_frame_ms;
        uint32_t load = frame_load_ms(frame);

        printf("Frame %2u [%4u - %4u ms] load %3u/%u ms : ",
               (unsigned int) frame,
               (unsigned int) start,
               (unsigned int) end,
               (unsigned int) load,
               (unsigned int) minor_frame_ms);

        if (slot_count[frame] == 0)
        {
            printf("(idle)\n");
            continue;
        }

        for (uint32_t slot = 0; slot < slot_count[frame]; slot++)
        {
            const CE_Task *task = CE_GetTask(task_slots[frame][slot]);

            printf("%s(%ums)", task->task_name, (unsigned int) task->wcet_ms);
            if (slot + 1U < slot_count[frame])
            {
                printf(", ");
            }
        }

        printf("\n");
    }

    printf("==============================================\n");
}

uint32_t CE_GetMajorFrame(void)
{
    return major_frame_ms;
}

uint32_t CE_GetMinorFrame(void)
{
    return minor_frame_ms;
}

uint32_t CE_GetFrameCount(void)
{
    return frame_count;
}

static void execute_frame(uint32_t frame, uint32_t cycle)
{
    TickType_t frame_start = xTaskGetTickCount();

    printf("\n[CE] cycle %u frame %u at %lu ms\n",
           (unsigned int) cycle,
           (unsigned int) frame,
           (unsigned long) frame_start);

    for (uint32_t slot = 0; slot < slot_count[frame]; slot++)
    {
        uint32_t index = task_slots[frame][slot];
        const CE_Task *task = CE_GetTask(index);

        task->task_function();
        jobs_executed[index]++;
    }

    if ((xTaskGetTickCount() - frame_start) > pdMS_TO_TICKS(minor_frame_ms))
    {
        printf("[CE] frame %u overran its %u ms budget\n",
               (unsigned int) frame,
               (unsigned int) minor_frame_ms);
    }
}

static void print_execution_summary(uint32_t completed_cycles)
{
    int pass = 1;

    printf("\n========== EXECUTION SUMMARY ==========\n");
    printf("Major cycles completed : %u\n", (unsigned int) completed_cycles);

    for (uint32_t index = 0; index < CE_GetTaskCount(); index++)
    {
        const CE_Task *task = CE_GetTask(index);
        uint32_t expected = completed_cycles * (major_frame_ms / task->period_ms);

        printf("%-15s expected %2u  executed %2u\n",
               task->task_name,
               (unsigned int) expected,
               (unsigned int) jobs_executed[index]);

        if (jobs_executed[index] != expected)
        {
            pass = 0;
        }
    }

    printf("Result : %s\n", pass ? "PASS" : "FAIL");
    printf("=======================================\n");
}

static void cyclic_executive_task(void *parameters)
{
    TickType_t wake_time;
    uint32_t cycles_to_run = configured_major_cycles;
    uint32_t cycle = 0;
    (void) parameters;

    wake_time = xTaskGetTickCount();

    for (;;)
    {
        for (uint32_t frame = 0; frame < frame_count; frame++)
        {
            execute_frame(frame, cycle);
            vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(minor_frame_ms));
        }

        cycle++;

        if (cycles_to_run != 0 && cycle >= cycles_to_run)
        {
            print_execution_summary(cycle);
            vTaskEndScheduler();
        }
    }
}

CE_Status CE_Start(uint32_t major_cycles)
{
    BaseType_t created;

    if (!schedule_ready)
    {
        return CE_ERROR_NOT_INITIALIZED;
    }

    configured_major_cycles = major_cycles;

    for (uint32_t index = 0; index < CE_MAX_TASKS; index++)
    {
        jobs_executed[index] = 0;
    }

    created = xTaskCreate(cyclic_executive_task,
                           "CyclicExec",
                           configMINIMAL_STACK_SIZE,
                           NULL,
                           CE_DISPATCHER_PRIORITY,
                           NULL);

    if (created != pdPASS)
    {
        return CE_ERROR_INVALID_ARGUMENT;
    }

    printf("\nStarting FreeRTOS cyclic executive");
    if (major_cycles == 0)
    {
        printf(" (runs until stopped).\n");
    }
    else
    {
        printf(" for %u major cycle(s).\n", (unsigned int) major_cycles);
    }

    vTaskStartScheduler();

    printf("\nScheduler stopped.\n");
    return CE_SUCCESS;
}
