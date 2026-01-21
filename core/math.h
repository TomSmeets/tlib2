// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// math.h - Math functions
#pragma once
#include "type.h"

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
