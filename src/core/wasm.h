// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// wasm.h - Generic js interop
#pragma once
#include "type.h"

#define WASM_PAGE_SIZE (64 * 1024)
static size_t wasm_memory_grow(size_t pages) {
    return __builtin_wasm_memory_grow(0, pages);
}

// Import a javascript function into WebAssembly
// To export functions just make them non-static
#define WASM_IMPORT(name) __attribute((import_module("env"), import_name(#name)))

// Run wasm function
WASM_IMPORT(js_push) void js_push_i32(i32 a);
WASM_IMPORT(js_push) void js_push_u32(u32 a);
WASM_IMPORT(js_push) void js_push_i64(i64 a);
WASM_IMPORT(js_push) void js_push_u64(u64 a);
WASM_IMPORT(js_push) void js_push_size(size_t a);
WASM_IMPORT(js_push) void js_push_ptr(void *a);
WASM_IMPORT(js_push_str) void js_push_str(char *a);
WASM_IMPORT(js_call) void js_call_void(char *code);
WASM_IMPORT(js_call) i32 js_call_i32(char *code);
WASM_IMPORT(js_call) i64 js_call_i64(char *code);

// clang-format off
#define JS_PUSH(x)           \
    _Generic((x),            \
        char *: js_push_str, \
        void *: js_push_ptr, \
        i32:    js_push_i32, \
        u32:    js_push_u32, \
        i64:    js_push_i64, \
        u64:    js_push_u64, \
        size_t: js_push_size \
    )(x)
// clang-format on

#define js_ret(code, return_type, ...) \
    ({ \
        REPEAT(JS_PUSH, __VA_ARGS__); \
        js_call_##return_type(code); \
    })

#define js(code, ...) js_ret(code, void, ##__VA_ARGS__)

static void js_append_style(char *css) {
    js(R"((css) => {
        let el = document.createElement('style');
        el.innerHTML = css;
        document.head.appendChild(el);
    })",
       css);
}

static void js_set_html(char *html) {
    js("(html) => document.body.innerHTML = html", html);
}
