// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// pix_wasm.h - Simple 2d game engine
#pragma once
#include "input.h"
#include "mem.h"
#include "pix_api.h"

struct Pix {};

// Create a new Pix renderer with a given title and window size
static Pix *pix_new(char *title, v2i window_size) {
    Memory *mem = mem_new();
    Pix *pix = mem_struct(mem, Pix);
    return pix;
}

// Destroy window and cleanup renderer
static void pix_free(Pix *pix);

// Poll input event
// - Keep polling until InputEvent_None
static Input pix_input(Pix *pix);

// Update screen contents
// - Pixels are r,g,b, from top left to bottom right
// - Image is scaled
static void pix_draw(Pix *pix, v2i size, u8 *rgb);

// Play a sound effect
// - sample rate is 48000 Hz
// - 2 channels
// - each sample is two 16 bit integers (left, right)
static void pix_play(Pix *pix, u32 sample_count, Pix_Audio_Sample *samples);
