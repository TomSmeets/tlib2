// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// vec.h - Vector type
#pragma once
#include "math.h"
#include "type.h"

typedef struct {
    i32 x;
    i32 y;
} v2i;

typedef struct {
    f32 x;
    f32 y;
} v2f;

static v2f v2f_clamp(v2f v, f32 min, f32 max) {
    v.x = f_clamp(v.x, min, max);
    v.y = f_clamp(v.y, min, max);
    return v;
}

static v2f v2f_scale(v2f v, f32 scale) {
    v.x *= scale;
    v.y *= scale;
    return v;
}
