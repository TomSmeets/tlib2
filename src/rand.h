// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// rand.h - 32 bit PCG random number generator
#pragma once
#include "math.h"
#include "type.h"

typedef struct {
    u64 state;
    u64 inc;
} Rand;

// Initialize a new pseudo random number generator with a given seed
static Rand rand_new(u64 seed) {
    return (Rand){.state = seed, .inc = 1};
}

// Generate a random 32 bit number
// range: 0 <= x < 2**32
// Based on "Really minimal PCG32 code" by (c) 2014 M.E. O'Neill / pcg-random.org
static u32 rand_next(Rand *rand) {
    rand->state = 6364136223846793005ULL * rand->state + (rand->inc | 1u);
    u32 xor_shifted = ((rand->state >> 18u) ^ rand->state) >> 27u;
    u32 rot = rand->state >> 59u;
    return (xor_shifted >> rot) | (xor_shifted << ((-rot) & 31));
}

// Split this rng into two 'timelines'
static Rand rand_fork(Rand *rand) {
    Rand new = {};
    new.state = rand->state ^ rand_next(rand);
    new.inc = (rand_next(rand) << 1) | 1;
    rand_next(&new);
    return new;
}

// Random float in range [min, max)
static f32 rand_f32(Rand *rand, f32 min, f32 max) {
    return (f32)rand_next(rand) / (f32)0x100000000 * (max - min) + min;
}

// Random unsigned in range [min, max)
static u32 rand_u32(Rand *rand, u32 min, u32 max) {
    return (u32)rand_f32(rand, min, max);
}

// Random signed in range [min, max)
static i32 rand_i32(Rand *rand, i32 min, i32 max) {
    return f_floor(rand_f32(rand, min, max));
}

// Random choice with a given chance
// chance = 0 -> Never
// chance = 1 -> Always
static bool rand_choice(Rand *rand, f32 chance) {
    return rand_f32(rand, 0, 1) < chance;
}
