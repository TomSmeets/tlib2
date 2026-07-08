// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// math.h - Math functions
#pragma once
#include "type.h"

#define M_PI 3.14159265358979323846264338327950288

// Round towards -Inf
static i32 f_floor(f32 x) {
    return (i32)x - (x < (i32)x);
}

// Round towards nearest integer
static i32 f_round(f32 x) {
    return f_floor(x + 0.5);
}

// Round towards 0
static i32 f_trunc(f32 x) {
    return (i32)x;
}

static f32 f_fract(f32 x) {
    return x - f_floor(x);
}

static f32 f_clamp(f32 in, f32 min, f32 max) {
    if (in < min) return min;
    if (in > max) return max;
    return in;
}

float sinf(float);
float sqrtf(float);
float expf(float);

static f32 f_sin(f32 x) {
    return sinf(x);
}

static f32 f_sqrt(f32 x) {
    return sqrtf(x);
}

static f32 f_exp(f32 x) {
    return expf(x);
}
