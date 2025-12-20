// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os.h - Operating System API
#pragma once
#include "os_api.h"

// Linux
#if __unix__
#define OS_LINUX 1
#define OS_WINDOWS 0
#define OS_WASM 0
#include "os_linux.h"

// Windows
#elif _WIN32
#define OS_LINUX 0
#define OS_WINDOWS 1
#define OS_WASM 0
#include "os_windows.h"

// Webassembly
#elif __wasm__
#define OS_LINUX 0
#define OS_WINDOWS 0
#define OS_WASM 1
#endif
