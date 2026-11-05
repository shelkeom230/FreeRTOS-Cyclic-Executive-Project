#include <stdio.h>

#include "user_tasks.h" 

void read_sensor(void) {
    printf("[USER TASK] Reading sensor...\n");
}

void update_display(void) {
    printf("[USER TASK] updating display...\n");
}

void calculate_temperature(void) {
    printf("[USER TASK] Calculating temperature...\n");
}

void send_data(void) {
    printf("[USER TASK] Sending data...\n");
}