// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// os_headers.h - Basic platform includes
#pragma once
#include "type.h"

// Simple ifdef macros,
// Why? Because it is more readable, clang-format will not indent '#if'
#if OS_WINDOWS
#define IF_WINDOWS(X) X
#else
#define IF_WINDOWS(X)
#endif

#if OS_LINUX
#define IF_LINUX(X) X
#else
#define IF_LINUX(X)
#endif

#if OS_WASM
#define IF_WASM(X) X
#else
#define IF_WASM(X)
#endif

// =============== Linux =================
#if OS_LINUX
#include "linux.h"
#endif

// =============== Windows =================
#if OS_WINDOWS
#pragma push_macro("TIME_MS")
#pragma push_macro("PATH_MAX")
#include <windows.h>
#pragma pop_macro("PATH_MAX")
#pragma pop_macro("TIME_MS")
#endif

// =============== WASM =================
#include "wasm.h"

// Convert a unix file descriptor to a opaque pointer
static void *fd_to_handle(i32 fd) {
    if (fd < 0) return 0;
    return (void *)((intptr_t)fd + 1);
}

// Convert a opaque pointer back to a unix file descriptor
static i32 fd_from_handle(void *handle) {
    if (!handle) return -1;
    return (i32)((intptr_t)handle - 1);
}
