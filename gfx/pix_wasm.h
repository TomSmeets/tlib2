// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// pix_wasm.h - Simple 2d game engine
#pragma once
#include "fmt.h"
#include "input.h"
#include "mem.h"
#include "pix_api.h"

struct Pix {
    Memory *mem;
    bool has_audio;

    // Audio
    u32 audio_cursor;
    Pix_Audio_Sample audio_buffer[1024 * 48 * 5];

    u32 event_read;
    u32 event_write;
    Input events[64];
};

WASM_IMPORT(pix_wasm_start_input) void pix_wasm_start_input(Pix *pix);

// Create a new Pix renderer with a given title and window size
static Pix *pix_new(char *title, v2i window_size) {
    Memory *mem = mem_new();

    Pix *pix = mem_struct(mem, Pix);
    pix->mem = mem;
    pix_wasm_start_input(pix);

    return pix;
}

// Destroy window and cleanup renderer
static void pix_free(Pix *pix) {
    mem_free(pix->mem);
}

// Poll input event
// - Keep polling until InputEvent_None
static Input pix_input(Pix *pix) {
    Input in = {};
    if (pix->event_read == pix->event_write) return in;

    in = pix->events[pix->event_read];
    pix->event_read = (pix->event_read + 1) % array_count(pix->events);
    return in;
}

// Update screen contents
// - Pixels are r,g,b, from top left to bottom right
// - Image is scaled
WASM_IMPORT(pix_wasm_draw) void pix_wasm_draw(u32 sx, u32 sy, u8 *data);
static void pix_draw(Pix *pix, v2i size, u8 *rgb) {
    pix_wasm_draw(size.x, size.y, rgb);
}

WASM_IMPORT(pix_wasm_start_audio) void pix_wasm_start_audio(Pix_Audio_Sample *buffer_data, u32 buffer_size, u32 *cursor);

// Play a sound effect
// - sample rate is 48000 Hz
// - 2 channels
// - each sample is two 16 bit integers (left, right)
static void pix_play(Pix *pix, u32 sample_count, Pix_Audio_Sample *samples) {
    pix_wasm_start_audio(pix->audio_buffer, array_count(pix->audio_buffer), &pix->audio_cursor);
    sample_count = MIN(sample_count, array_count(pix->audio_buffer));
    u32 cursor = pix->audio_cursor;
    for (u32 i = 0; i < sample_count; ++i) {
        pix->audio_buffer[cursor].left += samples[i].left;
        pix->audio_buffer[cursor].right += samples[i].right;
        if (++cursor == array_count(pix->audio_buffer)) cursor = 0;
    }
}

static Key key_from_char(u32 key) {
    if (key >= 'a' && key <= 'z') return key - 'a' + Key_A;
    if (key >= 'A' && key <= 'Z') return key - 'A' + Key_A;
    if (key >= '0' && key <= '9') return key - '0' + Key_0;
    if (key == ' ') return Key_Space;
    return Key_None;
}

void pix_wasm_key_down(Pix *pix, u32 key, bool down) {
    u32 next_write = (pix->event_write + 1) % array_count(pix->events);
    if (next_write == pix->event_read) return;
    Input *ev = pix->events + pix->event_write;
    pix->event_write = next_write;

    if (down) {
        ev->type = InputEvent_KeyDown;
        ev->key_down = key_from_char(key);
    } else {
        ev->type = InputEvent_KeyUp;
        ev->key_up = key_from_char(key);
    }
}
