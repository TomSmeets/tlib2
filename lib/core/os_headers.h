// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// type.h - Basic os header includes
#pragma once
#include "type.h"

#if OS_LINUX
#include "linux.h"
#elif OS_WINDOWS
#pragma push_macro("TIME_MS")
#include <windows.h>
#pragma pop_macro("TIME_MS")
#elif OS_WASM
#define WASM_IMPORT(name) __attribute((import_module("env"), import_name(#name)))
#define WASM_PAGE_SIZE (64 * 1024)

static size_t wasm_memory_grow(size_t pages) {
    return __builtin_wasm_memory_grow(0, pages);
}
#endif
