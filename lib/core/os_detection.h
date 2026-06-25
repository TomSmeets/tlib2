// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// os_detection.h - Detects target OS
#pragma once

// Linux
#if __unix__
#define OS_LINUX 1
#define IF_LINUX(X) X
#else
#define OS_LINUX 0
#define IF_LINUX(X)
#endif

// Windows
#if _WIN32
#define OS_WINDOWS 1
#define IF_WINDOWS(X) X
#else
#define OS_WINDOWS 0
#define IF_WINDOWS(X)
#endif

// WebAssembly
#if __wasm__
#define OS_WASM 1
#define IF_WASM(X) X
#else
#define OS_WASM 0
#define IF_WASM(X)
#endif

#if !(OS_LINUX || OS_WINDOWS || OS_WASM)
#error Unsupported Platform
#endif
