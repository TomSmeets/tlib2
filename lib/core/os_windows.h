// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_linux.h - Linux syscalls
#pragma once
#include "error.h"
#include "os_api.h"
#include "os_headers.h"
#include "str.h"
#include "type.h"

// ==================================
//      File and Stream handling
// ==================================

// =================================
// ==== System Commands ====
static void os_system(char *command) {
    check(system(command) == 0);
}

// Execute a system command, returns the exit code
// - First argument is the process to execute
static i32 os_spawn(u32 argc, char **argv) {
    return -1;
}

// ==================================
//            File watcher
// ==================================
static Watch *os_watch_new(void) {
    // Not supported
    return 0;
}

static bool os_watch_add(Watch *watch, char *path) {
    // Not supported
    return false;
}

static bool os_watch_check(Watch *watch) {
    // Not supported
    return 0;
}
