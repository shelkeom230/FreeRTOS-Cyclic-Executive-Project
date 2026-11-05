# Generic Cyclic Executive on the FreeRTOS POSIX Simulator

A cyclic executive runs a fixed table of periodic jobs. This project is that table, built for the FreeRTOS Linux simulator. User work is registered with a function pointer. FreeRTOS only provides the clock and one dispatcher task. The user functions are not FreeRTOS tasks.

## Build and run

- go inside the CyclicExecutive Directory and run 

```bash
make
./build/cyclic_executive
```

`make clean` deletes `build/`. 
The demo executes two major cycles (2 seconds of simulated time) and then stops. Change `CE_DEMO_MAJOR_CYCLES` in `src/main.c` to run longer. A value of `0` runs until the process is stopped.

## Task model

Each task is a record in `include/task_config.h`:

| Field | Meaning |
| --- | --- |
| `task_name` | Name printed in the schedule |
| `task_function` | `void (*)(void)` job, called by the dispatcher |
| `period_ms` | Release period. The deadline is the same as the period |
| `wcet_ms` | Worst-case execution time, used for admission control |

`CE_RegisterTask()` rejects a null function, a zero period, or a WCET larger than the period. The sample set is registered in `src/main.c`:

| Task | Period | WCET | Function |
| --- | ---: | ---: | --- |
| Sensor | 100 ms | 10 ms | `read_sensor` |
| Display | 200 ms | 20 ms | `update_display` |
| Temperature | 500 ms | 30 ms | `calculate_temperature` |
| Data | 1000 ms | 40 ms | `send_data` |

Add another task by writing a `void` function and calling `CE_RegisterTask` before `CE_BuildSchedule`.

## Major frame and Minor frame

Implemented in `src/frame_calculator.c`.

The major frame is the hyperperiod, the least common multiple of the periods:

```python
H = LCM(100, 200, 500, 1000) = 1000 ms
```

The minor frame `f` is the smallest duration that satisfies all three rules:

1. `f` is at least as large as every WCET, so one job fits in one frame. Here `f >= 40`.
2. `H` is an integer multiple of `f`, so the table repeats exactly.
3. `2f - gcd(period, f) <= period` for every task, so there is always a whole frame between a release and its deadline.

The search accepts `f = 40`. The major frame therefore contains `1000 / 40 = 25` minor frames.

## Task-to-frame assignment

- Implemented in `src/scheduler.c` (`CE_BuildSchedule`).

- A job released at time `r` with deadline `r + period` may run only in a frame that starts at or after `r` and ends at or before the deadline. Among those frames, the scheduler picks the earliest one whose jobs still fit:

```text
load(frame) + WCET <= minor frame
```

- If any job has no such frame, schedule construction fails and the program does not start the kernel. For the sample set the table begins:

| Frame | Window | Jobs |
| ---: | --- | --- |
| 0 | 0–40 ms | Sensor, Display |
| 1 | 40–80 ms | Temperature |
| 2 | 80–120 ms | Data |
| 3 | 120–160 ms | Sensor |
| 4 | 160–200 ms | idle |
| 5 | 200–240 ms | Sensor, Display |

- `CE_PrintSchedule()` prints the full table before the kernel starts. Sensor’s second job is released at 100 ms, so it cannot use the frame that started at 80 ms. It runs in the frame that starts at 120 ms and ends at 160 ms, which is still before the deadline at 200 ms.

## FreeRTOS engine

- `CE_Start()` creates one FreeRTOS task, `CyclicExec`, and calls `vTaskStartScheduler()`.

- That task walks the schedule. At the start of a minor frame it calls each assigned function pointer. It then calls `vTaskDelayUntil()` for `minor_frame` milliseconds. `configTICK_RATE_HZ` is 1000, so one tick is one millisecond. `vTaskDelayUntil` measures from the previous frame boundary, so time spent inside the jobs is part of the frame rather than extra delay.

- After the requested number of major cycles the dispatcher prints an execution check and calls `vTaskEndScheduler()`. The POSIX port then returns from `vTaskStartScheduler()` into `main`.

## Test

- The check compares executions against the hyperperiod. In two major cycles the expected counts are:

| Task | Calculation | Expected |
| --- | --- | ---: |
| Sensor | 2 × (1000 / 100) | 20 |
| Display | 2 × (1000 / 200) | 10 |
| Temperature | 2 × (1000 / 500) | 4 |
| Data | 2 × (1000 / 1000) | 2 |

- A run of `./build/cyclic_executive` prints `Result : PASS` and frame timestamps `0, 40, 80, ...` ms. An overrun message is printed if the jobs in a frame take longer than the minor frame. The sample jobs only print, so their registered WCET is the admission-control budget, not a busy-wait.

## Files

| File | Role |
| --- | --- |
| `include/task_config.h` | Task record and function-pointer type |
| `include/cyclic_executive.h` | Platform API |
| `include/frame_calculator.h` | GCD, LCM, frame-size API |
| `src/cyclic_executive.c` | Init and task registration |
| `src/frame_calculator.c` | Major and minor frame calculation |
| `src/scheduler.c` | Frame assignment and FreeRTOS dispatcher |
| `src/main.c` | Demo task set, hooks, startup |
| `user_tasks/` | Sample jobs |
| `FreeRTOSConfig.h` | POSIX simulator configuration |
| `Makefile` | Build |
