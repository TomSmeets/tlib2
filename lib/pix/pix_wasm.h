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


#define STR(...) #__VA_ARGS__
// Create a new Pix renderer with a given title and window size
static Pix *pix_new(Memory *mem, char *title, v2i window_size) {
    Pix *pix = mem_struct(mem, Pix);
    pix->mem = mem;

    // Init
    // clang-format off
    wasm_call_vp(STR((pix) => {
        document.addEventListener("keydown", (ev) => {
            if (!ev.repeat) tlib.import.pix_wasm_key_down(pix, ev.keyCode, true);
        });
        document.addEventListener("keyup", (ev) => {
            if (!ev.repeat)  tlib.import.pix_wasm_key_down(pix, ev.keyCode, false);
        });
        document.addEventListener("contextmenu", (ev) => ev.preventDefault());
    }), pix);
    // clang-format on

    return pix;
}

// Destroy window and cleanup renderer
static void pix_quit(Pix *pix) {
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
static void pix_draw(Pix *pix, v2i size, u8 *rgb) {
    wasm_call_viip(STR((size_x, size_y, data_ptr) => {
        var data = new Uint8ClampedArray(tlib.memory.buffer, data_ptr, size_x*size_y*4);
        let image_data = new ImageData(data, size_x, size_y);

        if(!tlib.canvas) {
            tlib.canvas = document.getElementById("canvas");
            tlib.canvas.width = size_x;
            tlib.canvas.height = size_y;
            tlib.ctx = tlib.canvas.getContext("2d");
        }

        if(tlib.canvas.width != size_x || tlib.canvas.height != size_y) {
            tlib.canvas.width = size_x;
            tlib.canvas.height = size_y;
        }
        tlib.ctx.putImageData(image_data, 0, 0);
    }), size.x, size.y, rgb);
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
    if (key == 37) return Key_Left;
    if (key == 38) return Key_Up;
    if (key == 39) return Key_Right;
    if (key == 40) return Key_Down;
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
