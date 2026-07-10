// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// vec.h - Vector type
#pragma once
#include "math.h"
#include "type.h"

// v2f
typedef f32 v2f __attribute__((ext_vector_type(2)));

// Clamp components between min and max
static v2f v2f_clamp(v2f v, f32 min, f32 max) {
    v.x = f_clamp(v.x, min, max);
    v.y = f_clamp(v.y, min, max);
    return v;
}

// Inner product
static f32 v2f_dot(v2f a, v2f b) {
    return a.x * b.x + a.y * b.y;
}

// Vector magnitude squared
static f32 v2f_length_sq(v2f a) {
    return v2f_dot(a, a);
}

// Vector magnitude
static f32 v2f_length(v2f a) {
    return f_sqrt(v2f_length_sq(a));
}

// ==================================================

// 3d float vector
typedef f32 v3f __attribute__((ext_vector_type(3)));

// Clamp components between min and max
static v3f v3f_clamp(v3f v, f32 min, f32 max) {
    v.x = f_clamp(v.x, min, max);
    v.y = f_clamp(v.y, min, max);
    v.z = f_clamp(v.z, min, max);
    return v;
}

// Inner product
static f32 v3f_dot(v3f a, v3f b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Vector magnitude squared
static f32 v3f_length_sq(v3f a) {
    return v3f_dot(a, a);
}

// Vector magnitude
static f32 v3f_length(v3f a) {
    return f_sqrt(v3f_length_sq(a));
}

// Vector Cross product
// cross(x,y) = z
// cross(y,z) = x
// cross(z,x) = y
static v3f v3f_cross(v3f a, v3f b) {
    v3f res;
    res.x = a.y * b.z - a.z * b.y;
    res.y = a.z * b.x - a.x * b.z;
    res.z = a.x * b.y - a.y * b.x;
    return res;
}

typedef f32 v4f __attribute__((ext_vector_type(4)));
typedef i32 v2i __attribute__((ext_vector_type(2)));
typedef i32 v3i __attribute__((ext_vector_type(3)));
typedef i32 v4i __attribute__((ext_vector_type(4)));
