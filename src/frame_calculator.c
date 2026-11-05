#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include "frame_calculator.h"

/* Calculate GCD using Euclidean algorithm */

uint32_t CE_GCD(uint32_t a, uint32_t b)
{
    while (b != 0)
    {
        uint32_t temp = b;
        b = a % b;
        a = temp;
    }

    return a;
}


/* Calculate LCM */

uint32_t CE_LCM(uint32_t a, uint32_t b)
{
    if (a == 0 || b == 0)
        return 0;

    uint64_t result = ((uint64_t)a / CE_GCD(a, b)) * b;

    if (result > UINT32_MAX)
        return 0;

    return (uint32_t)result;
}


/*
 * Major frame  = hyperperiod = LCM of every task period.
 * Minor frame f is the smallest duration that satisfies all three rules
 * (deadline is taken to be the period):
 *   1. f >= every task WCET, so a job fits inside one frame
 *   2. the major frame is an integer number of minor frames
 *   3. 2f - gcd(period, f) <= period, so a whole frame lies between
 *      a job's release and its deadline
 */
CE_Status CE_CalculateFrames(
    uint32_t *major_frame,
    uint32_t *minor_frame,
    uint32_t *number_of_frames)
{
    uint32_t task_count = CE_GetTaskCount();

    if (major_frame == NULL ||
        minor_frame == NULL ||
        number_of_frames == NULL ||
        task_count == 0)
    {
        return CE_ERROR_INVALID_ARGUMENT;
    }

    uint32_t major = 1;
    uint32_t max_wcet = 0;

    /* Step 1: Calculate major frame */

    for (uint32_t i = 0; i < task_count; i++)
    {
        const CE_Task *task = CE_GetTask(i);

        if (task == NULL ||
            task->period_ms == 0 ||
            task->wcet_ms == 0)
        {
            return CE_ERROR_INVALID_ARGUMENT;
        }

        major = CE_LCM(major, task->period_ms);

        if (major == 0)
            return CE_ERROR_INVALID_ARGUMENT;

        if (task->wcet_ms > max_wcet)
            max_wcet = task->wcet_ms;
    }

    /* Step 2: Find the smallest valid minor frame */

    uint32_t minor = 0;

    for (uint32_t f = max_wcet; f <= major; f++)
    {
        if (major % f != 0)
            continue;

        int valid = 1;

        for (uint32_t i = 0; i < task_count; i++)
        {
            const CE_Task *task = CE_GetTask(i);

            uint32_t g = CE_GCD(task->period_ms, f);

            uint64_t constraint = 2ULL * f - g;

            /* Assume deadline = period */

            if (constraint > task->period_ms)
            {
                valid = 0;
                break;
            }
        }

        if (valid)
        {
            minor = f;
            break;
        }

        /* Avoid unsigned integer overflow */

        if (f == UINT32_MAX)
            break;
    }

    if (minor == 0)
        return CE_ERROR_INVALID_ARGUMENT;

    /* Step 3: Store results */

    *major_frame = major;
    *minor_frame = minor;
    *number_of_frames = major / minor;

    return CE_SUCCESS;
}