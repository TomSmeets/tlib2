// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// main.c - Game entrypoint
#include "alsa.h"
#include "audio.h"
#include "fmt.h"
#include "macro.h"
#include "math.h"
#include "os_main.h"
#include "sound.h"
#include "type.h"
#include "vec.h"

// Generate audio sample
static v2f sample(Sound *snd) {
    f32 p = sound_sin(snd, .5, 0);
    f32 v1 = sound_sin(snd, 400 + 10 * p, 0);
    f32 v2 = sound_sin(snd, 200, v1);
    f32 v3 = sound_noise_white(snd);

    f32 p1 = f_clamp(p, 0, 1);
    f32 p2 = f_clamp(-p, 0, 1);

    Freeverb_Config cfg = {
        .room = 0.8f,
        .damp = 0.2f,
        .wet = 0.5f,
        .dry = 1.0f,
    };
    v2f out = {};

    // snowstorm
    out += ((v2f){1, 1}) * sound_filter(snd, 400 + sound_sin(snd, 0.02, 0) * 100, v3).band_pass;
    // out += sound_sin(snd, NOTE_C * OCT_4, sound_sin(snd, NOTE_A * OCT_0, 0));

    // out += sound_filter(snd, 800, sound_noise_white(snd) * f_max(sound_sin(snd, 1, 0) * 4 - 3, 0) * 2).band_pass;
    return out;
}

static bool init;
static Audio audio;
static Sound snd = {};

static void os_main(void) {
    if (!init) {
        audio = audio_open();
        init = 1;
    }

    i16 samples[AUDIO_BUFFER_SIZE * 2];
    f32 volume = 0.1;
    for (u32 i = 0; i < array_count(samples);) {
        sound_start(&snd);
        v2f out = v2f_clamp(sample(&snd) * volume, -1, 1) * 0x7fff;
        samples[i++] = (i16)out.x;
        samples[i++] = (i16)out.y;
    }
    audio_play(&audio, samples, array_count(samples) / 2);
}
