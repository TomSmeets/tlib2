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
    wasm_call_vp(
        R"((pix) => {
            document.addEventListener("keydown", (ev) => {
                if (!ev.repeat) {
                    tlib.import.pix_wasm_key_down(pix, ev.keyCode, true);
                }
            });
            document.addEventListener("keyup", (ev) => {
                if (!ev.repeat) {
                    tlib.import.pix_wasm_key_down(pix, ev.keyCode, false);
                }
            });
            document.addEventListener("contextmenu", (ev) => {
                ev.preventDefault();
            });
        })",
        pix
    );

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
    wasm_call_viip(
        R"((size_x, size_y, data_ptr) => {
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
        })",
        size.x, size.y, rgb
    );
}

// Play a sound effect
// - sample rate is 48000 Hz
// - 2 channels
// - each sample is two 16 bit integers (left, right)
static void pix_play(Pix *pix, u32 sample_count, Pix_Audio_Sample *samples) {
    wasm_call_vpip(
        R"((buffer_data_ptr, buffer_size, cursor_ptr) => {
            if(!tlib.audio_init) {
                const buffer_data = new Float32Array(tlib.memory.buffer, buffer_data_ptr, buffer_size*2);
                const cursor      = new Uint32Array(tlib.memory.buffer, cursor_ptr, 1);
                const sample_count = 1024;
                tlib.audio = new AudioContext();
                tlib.audio_processor = tlib.audio.createScriptProcessor(sample_count, 2, 2);
                tlib.audio_processor.onaudioprocess = function (ev) {
                    const chan_0 = ev.outputBuffer.getChannelData(0);
                    const chan_1 = ev.outputBuffer.getChannelData(1);
                    for(let i = 0; i < sample_count; ++i) {
                        const c0 = cursor[0]*2 + 0;
                        const c1 = cursor[0]*2 + 1;

                        // Read sample
                        chan_0[i] = buffer_data[c0];
                        chan_1[i] = buffer_data[c1];

                        // Clear sample
                        buffer_data[c0] = 0;
                        buffer_data[c1] = 0;

                        // Advance cursor
                        cursor[0] += 1;
                        if (cursor[0] >= buffer_size) cursor[0] = 0;
                    }
                }
                tlib.audio_processor.connect(tlib.audio.destination);
                tlib.audio_init = true;
            }
            tlib.audio.resume();
        })",
        pix->audio_buffer, array_count(pix->audio_buffer), &pix->audio_cursor
    );

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
