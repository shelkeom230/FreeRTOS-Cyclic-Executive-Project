#ifndef FRAME_CALCULATOR_H
#define FRAME_CALCULATOR_H

#include <stdint.h>

#include "cyclic_executive.h"

uint32_t CE_GCD(uint32_t a, uint32_t b);

uint32_t CE_LCM(uint32_t a, uint32_t b);

CE_Status CE_CalculateFrames(
    uint32_t *major_frame,
    uint32_t *minor_frame,
    uint32_t *number_of_frames
);

#endif