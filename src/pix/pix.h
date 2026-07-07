// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// pix.h - Simple 2d game engine
#pragma once
#include "pix_api.h"

#if OS_LINUX || OS_WINDOWS
#include "pix_sdl.h"
#elif OS_WASM
#include "pix_wasm.h"
#else
#error "Unknown platform"
#endif
