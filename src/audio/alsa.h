// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// alsa.h - Simple alsa audio api wrapper
#pragma once
#include "dl.h"
#include "type.h"

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
    int (*snd_pcm_set_params)(
        snd_pcm_t *pcm, snd_pcm_format_t format, snd_pcm_access_t access, uint channels, uint rate, int soft_resample, uint latency
    );

    // Returns a positive number of frames actually written otherwise a negative error code
    long (*snd_pcm_writei)(snd_pcm_t *pcm, const void *buffer, ulong size);

    // Returns a positive number of frames actually read otherwise a negative error code
    long (*snd_pcm_readi)(snd_pcm_t *pcm, void *buffer, ulong size);
} Alsa_API;

static void alsa_load(Alsa_API *api) {
    Library *handle = dl_open("libasound.so");
    api->snd_pcm_open = dl_sym(handle, "snd_pcm_open");
    api->snd_pcm_drain = dl_sym(handle, "snd_pcm_drain");
    api->snd_pcm_close = dl_sym(handle, "snd_pcm_close");
    api->snd_pcm_set_params = dl_sym(handle, "snd_pcm_set_params");
    api->snd_pcm_writei = dl_sym(handle, "snd_pcm_writei");
    api->snd_pcm_readi = dl_sym(handle, "snd_pcm_readi");
}
