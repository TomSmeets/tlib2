// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// os_detection.h - Detects target OS
#pragma once

// OS Detection
// Linux
#if __unix__
#define OS_LINUX 1
#define OS_WINDOWS 0
#define OS_WASM 0

// Windows
#elif _WIN32
#define OS_LINUX 0
#define OS_WINDOWS 1
#define OS_WASM 0

// WebAssembly
#elif __wasm__
#define OS_LINUX 0
#define OS_WINDOWS 0
#define OS_WASM 1
#endif
