// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// sound_var.h - Immediate mode sound synthesis
#pragma once
#include "audio.h"
#include "macro.h"
#include "rand.h"
#include "type.h"

typedef struct {
    Rand rng;
    u32 value_index;
    f32 value_list[1 * 1024 * 1024];
} Sound;

// Start next sample
static void sound_start(Sound *s) {
    s->value_index = 0;
}

// Get N persistent variables
static f32 *sound_vars(Sound *s, u32 n) {
    if (s->value_index + n >= array_count(s->value_list)) return 0;
    f32 *ret = s->value_list + s->value_index;
    s->value_index += n;
    return ret;
}

// Get a persistent variable
static f32 *sound_var(Sound *s) {
    return sound_vars(s, 1);
}

// Get a persistent integer variable
static u32 *sound_u32(Sound *s) {
    return (u32 *)sound_var(s);
}

// Create a 0-1 step at a given frequency
static f32 sound_step(Sound *s, f32 freq) {
    f32 *t = sound_var(s);
    f32 v = *t;
    *t += AUDIO_DT * freq;
    *t -= (i32)*t;
    return v;
}

// Update value when input is true, otherwise return previous input
static f32 sound_hold(Sound *snd, bool set, f32 input) {
    f32 *v = sound_var(snd);
    if (set) *v = input;
    return *v;
}
