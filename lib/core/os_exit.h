// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// os_exit.h
#pragma once
#include "error.h"
#include "os_headers.h"
#include "str.h"

// Exit current application with optionally an error
static void os_exit(void) __attribute__((__noreturn__));

#if OS_LINUX
static void os_exit(void) {
    if (error) {
        linux_write(2, error, str_len(error));
        linux_exit_group(1);
    } else {
        linux_exit_group(0);
    }
    __builtin_trap();
}
#elif OS_WINDOWS
static void os_exit(void) {
    if (error) {
        MessageBoxA(NULL, error, "Error", MB_ICONERROR | MB_OK);
        ExitProcess(1);
    } else {
        ExitProcess(0);
    }
    __builtin_trap();
}
#elif OS_WASM
WASM_IMPORT(wasm_exit) void wasm_exit(char *message, u32 len);
static void os_exit(void) {
    wasm_exit(error, str_len(error));
    __builtin_trap();
}
#endif
