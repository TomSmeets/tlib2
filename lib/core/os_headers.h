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
WASM_IMPORT(wasm_call) void wasm_call(char *);
WASM_IMPORT(wasm_call) i32 wasm_call_i(char *);
WASM_IMPORT(wasm_call) i32 wasm_call_ii(char *, i32);
WASM_IMPORT(wasm_call) void wasm_call_vi(char *, i32);
WASM_IMPORT(wasm_call) void wasm_call_vp(char *, void *);
WASM_IMPORT(wasm_call) void wasm_call_vpi(char *, void *, i32);
WASM_IMPORT(wasm_call) void wasm_call_viip(char *, i32, i32, void *);
WASM_IMPORT(wasm_call) void wasm_call_vpip(char *, void *, i32, void *);

WASM_IMPORT(wasm_call) i64 wasm_call_l(char *);
WASM_IMPORT(wasm_call) i64 wasm_call_ll(char *, i64);
WASM_IMPORT(wasm_call) void wasm_call_vl(char *, i64);
WASM_IMPORT(wasm_call) void wasm_call_vpl(char *, void *, i64);
#else
#define WASM_IMPORT(name)
void wasm_call(char *) {}
i32 wasm_call_i(char *) {return 0;}
i32 wasm_call_ii(char *, i32) {return 0;}
void wasm_call_vi(char *, i32) {}
void wasm_call_vp(char *, void *) {}
void wasm_call_vpi(char *, void *, i32) {}
void wasm_call_viip(char *, i32, i32, void *) {}
void wasm_call_vpip(char *, void *, i32, void *) {}

i64 wasm_call_l(char *) {return 0;}
i64 wasm_call_ll(char *, i64) {return 0;}
void wasm_call_vl(char *, i64) {}
void wasm_call_vpl(char *, void *, i64) {}
#endif

