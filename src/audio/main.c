// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// main.c - Game entrypoint
#include "alsa.h"
#include "audio.h"
#include "macro.h"
#include "math.h"
#include "os_main.h"
#include "sound.h"
#include "type.h"
#include "vec.h"

// Generate audio sample
static v2f sample(Sound *snd) {
    f32 v1 = snd_saw(snd, 400);
    f32 v2 = snd_saw(snd, 200);

    f32 p = snd_sin(snd, .5);
    f32 p1 = f_clamp(p, 0, 1);
    f32 p2 = f_clamp(-p, 0, 1);

    v2f out = {};
    out.x = v1 * p1 + v2 * p2;
    out.y = v1 * p2 + v2 * p1;
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
        snd_start(&snd);
        v2f out = sample(&snd);
        out = v2f_scale(out, volume);
        out = v2f_clamp(out, -1, 1);
        out = v2f_scale(out, (f32)0x7fff);
        samples[i++] = (i16)out.x;
        samples[i++] = (i16)out.y;
    }
    audio_play(&audio, samples, array_count(samples) / 2);
}
