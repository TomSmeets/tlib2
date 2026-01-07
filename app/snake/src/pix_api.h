// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// pix.h - Simple 2d game engine
#pragma once
#include "input.h"

typedef struct Pix Pix;

// Create a new Pix renderer with a given title and window size
static Pix *pix_new(char *title, v2i window_size);

// Destroy window and cleanup renderer
static void pix_free(Pix *pix);

// Poll input event
// - Keep polling until InputEvent_None
static Input pix_input(Pix *pix);

// Update screen contents
// - Pixels are r,g,b, from top left to bottom right
// - Image is scaled
static void pix_draw(Pix *pix, v2i size, u8 *rgb);

typedef struct {
    i16 left;
    i16 right;
} Pix_Audio_Sample;

// Play a sound effect
// - sample rate is 48000 Hz
static void pix_play(Pix *pix, u32 sample_count, Pix_Audio_Sample *audio);
