# Cyclic Executive on FreeRTOS

A generic cyclic executive for the FreeRTOS POSIX (Linux) simulator. Periodic jobs are registered with a function pointer, a period, and a worst-case execution time. The platform computes the major and minor frames, assigns each job to a frame, and runs the table from one FreeRTOS dispatcher task.

## Build and run

From this directory:

```bash
make
./build/cyclic_executive
```

The demo runs two major cycles and prints `Result : PASS`. `make clean` removes the build output.

## Layout

| Path | Purpose |
| --- | --- |
| `src/cyclic_executive.c` | Task registration |
| `src/frame_calculator.c` | Major and minor frame calculation |
| `src/scheduler.c` | Task-to-frame assignment and the FreeRTOS engine |
| `src/main.c` | Sample tasks and startup |
| `user_tasks/` | Sample job functions |
| `ASSIGNMENT.md` | Design notes for the frame rules, schedule, and test |

To add a job, write a `void` function and call `CE_RegisterTask()` in `src/main.c` before `CE_BuildSchedule()`.
