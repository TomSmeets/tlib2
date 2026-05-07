// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_wasm.h - WASM OS abstraction
#pragma once
#include "error.h"
#include "os_api.h"
#include "os_headers.h"
#include "str.h"
#include "type.h"

// ==================================
//      File and Stream handling
// ==================================

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
