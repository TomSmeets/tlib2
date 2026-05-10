// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// os_headers.h - Basic platform includes
#pragma once
#include "type.h"

#if OS_LINUX
#include "linux.h"
#endif

#if OS_WINDOWS
#pragma push_macro("TIME_MS")
#pragma push_macro("PATH_MAX")
#include <windows.h>
#pragma pop_macro("PATH_MAX")
#pragma pop_macro("TIME_MS")
#endif

#if OS_WASM
#define WASM_IMPORT(name) __attribute((import_module("env"), import_name(#name)))
#define WASM_PAGE_SIZE (64 * 1024)
static size_t wasm_memory_grow(size_t pages) {
    return __builtin_wasm_memory_grow(0, pages);
}

WASM_IMPORT(wasm_call) void js_v(char *);
WASM_IMPORT(wasm_call) void js_vp(char *, void *);
WASM_IMPORT(wasm_call) void js_vpi(char *, void *, i32);
WASM_IMPORT(wasm_call) void js_vi(char *, i32);
WASM_IMPORT(wasm_call) void js_vl(char *, i64);
WASM_IMPORT(wasm_call) void js_vpip(char *, void *, i32, void *);
WASM_IMPORT(wasm_call) void js_viip(char *, i32, i32, void *);
WASM_IMPORT(wasm_call) i64 js_l(char *);
#else
// clang-format off
static void js_v(char *) {}
static void js_vp(char *, void *) {}
static void js_vpi(char *, void *, i32){};
static void js_vi(char *, i32) {}
static void js_vl(char *, i64) {}
static void js_vpip(char *, void *, i32, void *) {}
static void js_viip(char *, i32, i32, void *) {}
static i64 js_l(char *) {return 0;}
// clang-format on
#endif

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
 
