// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// alsa.h - Simple audio audio api wrapper
#pragma once
#include "os_detection.h"

#if OS_LINUX
#include "alsa.h"
#endif

// Samples per second
#define AUDIO_RATE 48000

// Seconds between samples
#define AUDIO_DT (1.0f / AUDIO_RATE)

// Number of samples to buffer (suggestion)
#define AUDIO_BUFFER_SIZE (AUDIO_RATE / 100)

typedef struct {
#if OS_LINUX
    u32 latency_ms;
    u32 channels;
    Alsa_API api;
    snd_pcm_t *pcm;
    snd_pcm_t *mic;
#endif
} Audio;

static Audio audio_open(void) {
    Audio audio = {};
    audio.latency_ms = 50;
    audio.channels = 2;
    alsa_load(&audio.api);
    return audio;
}

static void audio_play(Audio *audio, i16 *samples, u32 count) {
    if (!audio->pcm) {
        check(audio->api.snd_pcm_open(&audio->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0) == 0);
        if (error) return;
        check(
            audio->api.snd_pcm_set_params(
                audio->pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, audio->channels, AUDIO_RATE, 1, audio->latency_ms * 1000
            ) == 0
        );
    }

    check(audio->api.snd_pcm_writei(audio->pcm, samples, count) == count);
}

static void audio_record(Audio *audio, i16 *samples, u32 count) {
    if (!audio->mic) {
        check_or(audio->api.snd_pcm_open(&audio->mic, "default", SND_PCM_STREAM_CAPTURE, 0) == 0) return;
        check(
            audio->api.snd_pcm_set_params(
                audio->mic, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, audio->channels, AUDIO_RATE, 1, audio->latency_ms * 1000
            ) == 0
        );
    }

    check(audio->api.snd_pcm_readi(audio->pcm, samples, count) == count);
}

static void audio_close(Audio *audio) {
    check(audio->api.snd_pcm_drain(audio->pcm) == 0);
    check(audio->api.snd_pcm_close(audio->pcm) == 0);
}
