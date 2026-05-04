// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_wasm.h - WASM OS abstraction
#pragma once
#include "error.h"
#include "os_api.h"
#include "str.h"
#include "type.h"

#define WASM_IMPORT(name) __attribute((import_module("env"), import_name(#name)))

WASM_IMPORT(wasm_exit) void wasm_exit(void);
WASM_IMPORT(wasm_fail) void wasm_fail(char *message, u32 len);
static void os_exit(void) {
    if (error) {
        wasm_fail(error, str_len(error));
    } else {
        wasm_exit();
    }
    __builtin_trap();
}

static File *wasm_file(u32 fd) {
    return (File *)(intptr_t)fd;
}

static u32 wasm_fd(File *file) {
    return (u32)(intptr_t)file;
}

// ==================================
//      File and Stream handling
// ==================================
// clang-format off
static File *os_stdin(void)  { return wasm_file(1); }
static File *os_stdout(void) { return wasm_file(2); }
static File *os_stderr(void) { return wasm_file(3); }
// clang-format on

WASM_IMPORT(wasm_time) time_t wasm_time(void);
static time_t os_time(void) {
    return wasm_time();
}

WASM_IMPORT(wasm_sleep) void wasm_sleep(time_t duration);
static void os_sleep(time_t duration) {
    if (duration < 0) duration = 0;
    wasm_sleep(duration);
}

static u64 os_rand(void) {
    return os_time();
}

WASM_IMPORT(wasm_system) i32 wasm_system(char *code, u32 len);
static void os_system(char *command) {
    check(wasm_system(command, str_len(command)) == 0);
}

static Process *os_exec(char **argv) {
    os_fail("Not implemented");
}

static i32 os_wait(Process *proc) {
    os_fail("Not implemented");
}

static Watch *os_watch_new(void) {
    os_fail("Not implemented");
}

static bool os_watch_add(Watch *watch, char *path) {
    os_fail("Not implemented");
}

static bool os_watch_check(Watch *watch) {
    os_fail("Not implemented");
}
