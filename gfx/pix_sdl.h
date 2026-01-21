#pragma once
#include "mem.h"
#include "pix_api.h"
#include "sdl2.h"

struct Pix {
    Memory *mem;
    SDL2 sdl;

    // Window
    v2i window_size;
    SDL_Window *window;
    SDL_Renderer *renderer;

    // Draw
    v2i texture_size;
    SDL_Texture *texture;

    // Audio
    int audio_device;
    u32 audio_count;
    Pix_Audio_Sample audio_buffer[48000 * 5];
};

// Create a new Pix renderer with a given title and window size
static Pix *pix_new(char *title, v2i window_size) {
    Memory *mem = mem_new();
    Pix *pix = mem_struct(mem, Pix);
    pix->mem = mem;
    sdl2_load(&pix->sdl);

    pix->sdl.SDL_InitSubSystem(SDL_INIT_EVENTS);
    pix->sdl.SDL_InitSubSystem(SDL_INIT_AUDIO);
    pix->sdl.SDL_InitSubSystem(SDL_INIT_VIDEO);
    pix->sdl.SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    pix->window_size = window_size;
    pix->window = pix->sdl.SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_size.x, window_size.y, 0);
    pix->renderer = pix->sdl.SDL_CreateRenderer(pix->window, -1, SDL_RENDERER_ACCELERATED);
    return pix;
}

// Destroy window and cleanup renderer
static void pix_free(Pix *pix) {
    if (pix->texture) pix->sdl.SDL_DestroyTexture(pix->texture);
    if (pix->audio_device) pix->sdl.SDL_CloseAudioDevice(pix->audio_device);
    pix->sdl.SDL_DestroyRenderer(pix->renderer);
    pix->sdl.SDL_DestroyWindow(pix->window);
    pix->sdl.SDL_Quit();
    mem_free(pix->mem);
}

static Key sdl_mouse_to_key(u32 button) {
    if (button == SDL_BUTTON_LEFT) return Key_Mouse_Left;
    if (button == SDL_BUTTON_MIDDLE) return Key_Mouse_Middle;
    if (button == SDL_BUTTON_RIGHT) return Key_Mouse_Right;
    if (button == SDL_BUTTON_X1) return Key_Mouse_Forward;
    if (button == SDL_BUTTON_X2) return Key_Mouse_Back;
    return Key_None;
}

static Key sdl_key_to_key(u32 key) {
    if (key >= SDLK_a && key <= SDLK_z) return key - SDLK_a + Key_A;
    if (key >= SDLK_0 && key <= SDLK_9) return key - SDLK_0 + Key_0;
    if (key == SDLK_SPACE) return Key_Space;
    if (key == SDLK_ESCAPE) return Key_Escape;
    if (key == SDLK_LCTRL || key == SDLK_RCTRL) return Key_Control;
    if (key == SDLK_LSHIFT || key == SDLK_RSHIFT) return Key_Shift;
    if (key == SDLK_LALT || key == SDLK_RALT) return Key_Alt;
    if (key == SDLK_LGUI || key == SDLK_RGUI) return Key_Win;
    if (key == SDLK_UP) return Key_Up;
    if (key == SDLK_DOWN) return Key_Down;
    if (key == SDLK_LEFT) return Key_Left;
    if (key == SDLK_RIGHT) return Key_Right;
    return Key_None;
}

// Poll input event
// - Keep polling until InputEvent_None
static Input pix_input(Pix *pix) {
    Input input;
    SDL_Event event;
    while (pix->sdl.SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            input.type = InputEvent_Quit;
            return input;
        }
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            input.type = InputEvent_MouseDown;
            input.mouse_down = sdl_mouse_to_key(event.button.button);
            if (input.mouse_down == Key_None) continue;
            return input;
        }
        if (event.type == SDL_MOUSEBUTTONUP) {
            input.type = InputEvent_MouseUp;
            input.mouse_up = sdl_mouse_to_key(event.button.button);
            if (input.mouse_up == Key_None) continue;
            return input;
        }
        if (event.type == SDL_KEYDOWN) {
            if (event.key.repeat) continue;
            input.type = InputEvent_KeyDown;
            input.key_down = sdl_key_to_key(event.key.keysym.sym);
            if (input.key_down == Key_None) continue;
            return input;
        }
        if (event.type == SDL_KEYUP) {
            if (event.key.repeat) continue;
            input.type = InputEvent_KeyUp;
            input.key_up = sdl_key_to_key(event.key.keysym.sym);
            if (input.key_up == Key_None) continue;
            return input;
        }
    }
    input.type = InputEvent_None;
    return input;
}

// Update screen contents
// - Pixels are r,g,b, from top left to bottom right
// - Image is scaled
static void pix_draw(Pix *pix, v2i size, u8 *rgb) {
    // Recreate texture if it is resized
    if (!pix->texture || pix->texture_size.x != size.x || pix->texture_size.y != size.y) {
        if (pix->texture) pix->sdl.SDL_DestroyTexture(pix->texture);
        pix->texture_size = size;
        pix->texture = pix->sdl.SDL_CreateTexture(pix->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, size.x, size.y);
        pix->sdl.SDL_SetTextureScaleMode(pix->texture, SDL_ScaleModeNearest);
    }

    // Render texture
    pix->sdl.SDL_UpdateTexture(pix->texture, 0, rgb, size.x * 3);
    pix->sdl.SDL_RenderCopy(pix->renderer, pix->texture, 0, 0);
    pix->sdl.SDL_RenderPresent(pix->renderer);
}

static void _pix_audio_callback(void *user, u8 *stream, int len) {
    Pix *pix = user;

    // Number of samples (one sample is 16 bit)
    u32 output_count = len / sizeof(Pix_Audio_Sample);
    Pix_Audio_Sample *output_buffer = (Pix_Audio_Sample *)stream;

    u32 consumed_count = output_count;
    if (consumed_count > pix->audio_count) consumed_count = pix->audio_count;

    // Copy queued audio samples
    for (u32 i = 0; i < consumed_count; ++i) {
        output_buffer[i] = pix->audio_buffer[i];
    }

    // Clear remaining output samples
    for (u32 i = consumed_count; i < output_count; ++i) {
        output_buffer[i] = (Pix_Audio_Sample){};
    }

    // Update internal audio queue
    if (consumed_count == 0) {
        // Nothing consumed, so nothing to move
    } else if (consumed_count == pix->audio_count) {
        // Everything is consumed, so no need to move
        pix->audio_count = 0;
    } else {
        // Some samples are consumed,
        // remove them and move the remaining samples to the start
        u32 remaining_count = pix->audio_count - consumed_count;
        for (u32 i = 0; i < remaining_count; ++i) {
            pix->audio_buffer[i] = pix->audio_buffer[i + consumed_count];
        }
        pix->audio_count = remaining_count;
    }
}

// Play a sound effect
static void pix_play(Pix *pix, u32 sample_count, Pix_Audio_Sample *samples) {
    if (!pix->audio_device) {
        // Init audio
        SDL_AudioSpec audio_spec = {
            .freq = 48000,
            .format = AUDIO_F32,
            .channels = 2,
            .callback = _pix_audio_callback,
            .userdata = pix,
        };
        pix->audio_device = pix->sdl.SDL_OpenAudioDevice(0, 0, &audio_spec, 0, 0);

        // Start playing audio (pause set to 0 means play)
        pix->sdl.SDL_PauseAudioDevice(pix->audio_device, 0);
    }

    pix->sdl.SDL_LockAudioDevice(pix->audio_device);

    // Limit sample count
    if (sample_count > array_count(pix->audio_buffer)) {
        sample_count = array_count(pix->audio_buffer);
    }

    // Reserve space for more samples if needed
    while (pix->audio_count < sample_count) {
        pix->audio_buffer[pix->audio_count++] = (Pix_Audio_Sample){0, 0};
    }

    // Add samples to output stream
    for (u32 i = 0; i < sample_count; ++i) {
        pix->audio_buffer[i].left += samples[i].left;
        pix->audio_buffer[i].right += samples[i].right;
    }

    pix->sdl.SDL_UnlockAudioDevice(pix->audio_device);
}
