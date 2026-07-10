#pragma once
#include "math.h"
#include "sound_var.h"

// Create a 0-1 step at a given frequency
static f32 sound_phase(Sound *snd, f32 freq, f32 offset) {
    f32 *phase = sound_var(snd);
    f32 ret = f_fract(*phase + offset);
    *phase = f_fract(*phase + AUDIO_DT * freq);
    return ret;
}

// Saw wave
static f32 sound_saw(Sound *snd, f32 freq, f32 phase) {
    return sound_phase(snd, freq, phase + 0.5f) * 2.0f - 1.0f;
}

// Pulse wave
// on for duty% of the time
// off for the rest of the time
// duty = 0.5 -> 50% on then 50% off
// duty = 0.0 -> only low
// duty = 1.0 -> only high
static f32 sound_pulse(Sound *sound, f32 freq, f32 offset, f32 duty) {
    return sound_phase(sound, freq, offset) < duty ? 1 : -1;
}

// Sine wave
static f32 sound_sin(Sound *s, f32 freq, f32 phase) {
    return f_sin2pi(sound_phase(s, freq, phase));
}

static f32 sound_triangle(Sound *sound, f32 freq, f32 offset) {
    f32 v = sound_phase(sound, freq, offset);
    return f_min(v * 4 - 1, 3 - v * 4);
}

// Static white noise
static f32 sound_noise_white(Sound *sound) {
    return rand_f32(&sound->rng, -1, 1);
}

// Noise at a given frequency
// Kind of like sound_pulse with random values
static f32 sound_noise_freq(Sound *sound, f32 freq, f32 duty) {
    f32 *value = sound_var(sound);
    f32 *time = sound_var(sound);
    *time += AUDIO_DT * freq;
    bool trigger = *time > 1.0f;
    *time = f_fract(*time);
    if (trigger) *value = rand_f32(&sound->rng, -1, 1);
    return *value;
}
