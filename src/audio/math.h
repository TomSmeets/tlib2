// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// math.h - Simple math
#pragma once
#include "type.h"
#include <math.h>

static f32 f32_fract(f32 x) {
    return x - (u32)x;
}

static f32 f32_clamp(f32 in, f32 min, f32 max) {
    if (in < min) return min;
    if (in > max) return max;
    return in;
}

static f32 f32_sin(f32 x) {
    return sinf(x);
}

static f32 f32_sqrt(f32 x) {
    return sqrtf(x);
}

static f32 f32_exp(f32 x) {
    return expf(x);
}
