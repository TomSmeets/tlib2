// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// alsa.h - Simple alsa audio api wrapper
#pragma once
#include "type.h"
#include "dl.h"

#if 0
#include <alsa/asoundlib.h>
#else
typedef struct snd_pcm_t snd_pcm_t;
typedef enum {
    SND_PCM_STREAM_PLAYBACK,
    SND_PCM_STREAM_CAPTURE,
} snd_pcm_stream_t;

typedef enum {
    SND_PCM_FORMAT_UNKNOWN = -1,
    SND_PCM_FORMAT_S16_LE = 2,
} snd_pcm_format_t;

typedef enum {
    SND_PCM_ACCESS_RW_INTERLEAVED = 3,
} snd_pcm_access_t;
#endif

typedef struct {
    int (*snd_pcm_open)(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode);
    int (*snd_pcm_drain)(snd_pcm_t *pcm);
    int (*snd_pcm_close)(snd_pcm_t *pcm);
    int (*snd_pcm_set_params)(snd_pcm_t *pcm, snd_pcm_format_t format, snd_pcm_access_t access, uint channels, uint rate, int soft_resample, uint latency);
    long (*snd_pcm_writei)(snd_pcm_t *pcm, const void *buffer, ulong size);
} Alsa_API;

static void alsa_load(Alsa_API *api) {
    Library *handle = dl_open("libasound.so");
    api->snd_pcm_open = dl_sym(handle, "snd_pcm_open");
    api->snd_pcm_drain = dl_sym(handle, "snd_pcm_drain");
    api->snd_pcm_close = dl_sym(handle, "snd_pcm_close");
    api->snd_pcm_set_params = dl_sym(handle, "snd_pcm_set_params");
    api->snd_pcm_writei = dl_sym(handle, "snd_pcm_writei");
}

// Samples per second
#define SAMPLE_RATE 48000

// Seconds per sample
#define SAMPLE_DT (1.0f / SAMPLE_RATE)

// Number of samples to buffer
#define AUDIO_BUFFER_SIZE (SAMPLE_RATE / 100)

typedef struct {
    Alsa_API api;
    snd_pcm_t *pcm;
} Alsa;

static Alsa alsa_open(void) {
    u32 latency_ms = 50;
    u32 channels = 2;
    Alsa alsa = {};
    alsa_load(&alsa.api);
    alsa.api.snd_pcm_open(&alsa.pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
    alsa.api.snd_pcm_set_params(alsa.pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, channels, SAMPLE_RATE, 1, latency_ms * 1000);
    return alsa;
}

static long alsa_play(Alsa *alsa, i16 *samples, u32 count) {
    return alsa->api.snd_pcm_writei(alsa->pcm, samples, count);
}

static void alsa_close(Alsa *alsa) {
    alsa->api.snd_pcm_drain(alsa->pcm);
    alsa->api.snd_pcm_close(alsa->pcm);
}
