#pragma once
#include "audio.h"
#include "macro.h"
#include "math.h"
#include "type.h"

typedef struct {
    u32 i;
    f32 t[1 * 1024 * 1024];
} Sound;

// Start next sample
static void snd_start(Sound *s) {
    s->i = 0;
}

// Get N persistent variables
static f32 *snd_vars(Sound *s, u32 n) {
    if (s->i + n >= array_count(s->t)) return 0;
    f32 *ret = s->t + s->i;
    s->i += n;
    return ret;
}

// Get a persistent variable
static f32 *snd_var(Sound *s) {
    return snd_vars(s, 1);
}

// Get a persistent integer variable
static u32 *snd_u32(Sound *s) {
    return (u32 *)snd_var(s);
}

// Create a 0-1 step at a given frequency
static f32 snd_step(Sound *s, f32 freq) {
    f32 *t = snd_var(s);
    f32 v = *t;
    *t += AUDIO_DT * freq;
    *t -= (i32)*t;
    return v;
}

// Saw wave
static f32 snd_saw(Sound *s, f32 freq) {
    return snd_step(s, freq) * 2 - 1;
}

// Sine wave
static f32 snd_sin(Sound *s, f32 freq) {
    return f_sin(snd_saw(s, freq) * M_PI);
}

// Simple low-pass filter
static f32 snd_lowpass(Sound *sound, f32 cutoff_freq, f32 sample) {
    f32 *value = snd_var(sound);
    f32 ret = *value;
    f32 a = 1.0f - f_exp(-AUDIO_DT * M_PI * 2.0 * cutoff_freq);
    *value += (sample - ret) * a;
    return ret;
}

// Delay by n samples, n should be a constant
static f32 snd_delay(Sound *s, f32 in, u32 n) {
    f32 *buffer = snd_vars(s, n);
    u32 *i = (u32 *)snd_var(s);
    (*i)++;
    if (*i >= n) *i = 0;
    f32 ret = buffer[*i];
    buffer[*i] = in;
    return ret;
}

// Calculate rms volume
static f32 snd_volume(Sound *snd, f32 input) {
    return f_sqrt(snd_lowpass(snd, 1, input * input));
}

static f32 snd_hold(Sound *snd, bool set, f32 input) {
    f32 *v = snd_var(snd);
    if (set) *v = input;
    return *v;
}

// Get frequency by zero counting
static f32 snd_freq(Sound *snd, f32 in) {
    f32 vol = snd_volume(snd, in);
    f32 *prev = snd_var(snd);
    f32 *count = snd_var(snd);
    u32 *time = snd_u32(snd);
    u32 n = AUDIO_RATE / 100;

    bool cross = (*prev > 0) != (in > 0);
    *prev = in;
    if (cross) (*count) += 0.5f * (f32)AUDIO_RATE / (f32)n;
    (*time)++;

    f32 *freq = snd_var(snd);
    if (*time > n) {
        if (vol > 1.0) *freq = *count;
        *count = 0;
        *time = 0;
    }

    // return *freq;
    return snd_lowpass(snd, 1, *freq);
}
