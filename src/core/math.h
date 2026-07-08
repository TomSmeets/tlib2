// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// math.h - Math functions
#pragma once
#include "type.h"

#define INF (1.0f / 0.0f)
#define PI 3.14159265358979323846264338327950288f
#define DEG_TO_RAD (PI / 180.0f)

// Return minimum value
static f32 f_min(f32 a, f32 b) {
    return a <= b ? a : b;
}

// Return maximum value
static f32 f_max(f32 a, f32 b) {
    return a >= b ? a : b;
}

// Clamp value between min and max
static f32 f_clamp(f32 in, f32 min, f32 max) {
    if (in < min) return min;
    if (in > max) return max;
    return in;
}

// Absolute value of x
static f32 f_abs(f32 x) {
    return (x < 0) ? -x : x;
}

// Round towards -Inf
static i32 f_floor(f32 x) {
    return (i32)x - (x < (i32)x);
}

// Round towards nearest integer
static i32 f_round(f32 x) {
    return f_floor(x + 0.5f);
}

// Round towards 0
static i32 f_trunc(f32 x) {
    return (i32)x;
}

// Return fractional part
static f32 f_fract(f32 x) {
    return x - f_floor(x);
}

// Wrap x between l and h
static f32 f_wrap(f32 x, f32 l, f32 h) {
    f32 range = h - l;
    return x - range * f_floor((x - l) / range);
}

// Linearly remap input range to an output range
static f32 f_remap(f32 x, f32 xl, f32 xh, f32 yl, f32 yh) {
    if (x <= xl) return yl;
    if (x >= xh) return yh;
    f32 y = (x - xl) / (xh - xl);
    return y * (yh - yl) + yl;
}

// Convert between float it its bits
typedef union {
    f32 value;
    u32 bits;
} f32_bits;

// Get convert raw bits to a float
static f32 f_from_bits(u32 bits) {
    return ((f32_bits)bits).value;
}

// Get convert a float to its raw bits
static u32 f_to_bits(f32 value) {
    return ((f32_bits)value).bits;
}

// Smooth step function from 0 to 1
static f32 f_smoothstep(f32 x) {
    if (x <= 0) return 0;
    if (x >= 1) return 1;
    return x * x * (3.0f - 2.0f * x);
}

// https://gist.github.com/petrsm/079de9396d63e00d5994a7cc936ae9c7
static f32 f_pow2(f32 x) {
    // 2^x = 2^xi * 2^xf
    // pow2f - polynomial approximation of e(x) on <0, 1) range
    f32 pi = f_floor(x);
    f32 pf = x - pi;
    f32 pow2i = f_from_bits((1 << 23) * ((i32)pi + 127));
    f32 pow2f = 1.3697664475809267e-2f;
    pow2f = pow2f * pf + 5.1690358205939469e-2f;
    pow2f = pow2f * pf + 2.4163844572498163e-1f;
    pow2f = pow2f * pf + 6.9296612266139567e-1f;
    pow2f = pow2f * pf + 1.000003704465937f;
    return pow2i * pow2f;
}

static f32 f_exp(f32 x) {
    // 2^(x*ln(2))
    return f_pow2(x * 1.4426950408889634f);
}

// Sine (input in multiples of 2pi)
// Very good and simple sine approximation
static f32 f_sin2pi(f32 x) {
    x = x - f_floor(x + .5f);
    f32 y = 8 * x - 16 * x * f_abs(x);
    return 0.225f * (y * f_abs(y) - y) + y;
}

// Cosine (input in multiples of 2pi)
static f32 f_cos2pi(f32 x) {
    return f_sin2pi(x + .25f);
}

// Sine (input in radians)
static f32 f_sin(f32 x) {
    return f_sin2pi(x / (2 * PI));
}

// Cosine (input in radians)
static f32 f_cos(f32 x) {
    return f_cos2pi(x / (2 * PI));
}

// fast tangent approximation
static f32 f_tan(f32 x) {
    return f_sin(x) / f_cos(x);
}

static f32 f_rsqrt(f32 x) {
    if (x <= 0) return 0;
    u32 i = f_to_bits(x);
    i = 0x5f3759df - (i >> 1);
    f32 y = f_from_bits(i);
    y *= 1.5f - x * 0.5f * y * y;
    y *= 1.5f - x * 0.5f * y * y;
    return y;
}

// Calculate sqrt(x)
static f32 f_sqrt(f32 x) {
    if (x <= 0) return 0;
    return x * f_rsqrt(x);
}

// fast inverse tangent approximation
static f32 f_atan(f32 x) {
    return 0.25f * PI * x - x * (f_abs(x) - 1) * (0.2447f + 0.0663f * f_abs(x));
}

// fast 2 component inverse tangent approximation
// calculates the angle of (x, y) to the x axis
static f32 f_atan2(f32 y, f32 x) {
    // edge cases
    if (x == 0 && y == 0) return 0;
    if (x == 0 && y > 0) return PI / 2;
    if (x == 0 && y < 0) return -PI / 2;
    if (y == 0 && x > 0) return 0;
    if (y == 0 && x < 0) return PI;

    // Top Right
    // The unit circle divided into 8 sections
    if (x > 0 && y > 0 && x >= y) return 0 + f_atan(y / x);
    if (x > 0 && y > 0 && x < y) return PI / 2 - f_atan(x / y);

    if (x < 0 && y > 0 && -x <= y) return PI / 2 + f_atan(-x / y);
    if (x < 0 && y > 0 && -x > y) return PI - f_atan(-y / x);

    if (x < 0 && y < 0 && -x >= -y) return -PI + f_atan(y / x);
    if (x < 0 && y < 0 && -x < -y) return -PI / 2 - f_atan(x / y);

    if (x > 0 && y < 0 && x <= -y) return -PI / 2 + f_atan(-x / y);
    if (x > 0 && y < 0 && x > -y) return 0 - f_atan(-y / x);
    return 0;
}

static f32 f_asin(f32 x) {
    f32 ax = f_abs(x);
    f32 r = 0.5f * PI - f_sqrt(1.0f - ax) * (1.5707288f + ax * (-0.2121144f + ax * (0.0742610f + ax * -0.0187293f)));
    return (x < 0.0f) ? -r : r;
}

static f32 f_acos(f32 x) {
    return 0.5f * PI - f_asin(x);
}

// ===== u32 =====

// Return minimum value
static u32 u_min(u32 a, u32 b) {
    return a <= b ? a : b;
}

// Return maximum value
static u32 u_max(u32 a, u32 b) {
    return a >= b ? a : b;
}

// Clamp value between min and max
static u32 u_clamp(u32 in, u32 min, u32 max) {
    if (in < min) return min;
    if (in > max) return max;
    return in;
}

// ===== i32 =====

// Return minimum value
static i32 i_min(i32 a, i32 b) {
    return a <= b ? a : b;
}

// Return maximum value
static i32 i_max(i32 a, i32 b) {
    return a >= b ? a : b;
}

// Clamp value between min and max
static i32 i_clamp(i32 in, i32 min, i32 max) {
    if (in < min) return min;
    if (in > max) return max;
    return in;
}

// Absolute value of x
static u32 i_abs(i32 x) {
    return (x < 0) ? -x : x;
}
