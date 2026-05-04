// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_linux.h - Linux syscalls
#pragma once
#include "error.h"
#include "os_api.h"
#include "str.h"
#include "type.h"
#include "os_headers.h"

// ==================================
//      File and Stream handling
// ==================================

// =================================

// Open a library by name or full path
static Library *os_dlopen(char *path) {
    return (Library *)LoadLibrary(path);
}

// Lookup a symbol in a library
static void *os_dlsym(Library *lib, char *sym) {
    return GetProcAddress((void *)lib, sym);
}

// Get base address of a library based on a pointer inside that library
static void *os_dlbase(void *ptr) {
    // Not supported
    return 0;
}

// ==== Time ====
static time_t os_time(void) {
    static thread_local u64 freq;
    static thread_local time_t offset;

    // Get perf counter
    LARGE_INTEGER big_count;
    assert(QueryPerformanceCounter(&big_count));
    u64 count = (u64)big_count.QuadPart;

    if (!freq) {
        // Get counter frequency
        LARGE_INTEGER big_freq;
        assert(QueryPerformanceFrequency(&big_freq));
        freq = (u64)big_freq.QuadPart / 1000 / 1000;
        assert(freq > 0);

        // Initialize offset to match system time
        FILETIME ft;
        GetSystemTimePreciseAsFileTime(&ft);
        u64 win_time = ((u64)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
        i64 unix_time = (time_t)(win_time / 10) - 11644473600000000LL;
        offset = unix_time - (time_t)(count / freq);
    }

    time_t time = (time_t)(count / freq) + offset;
    return time;
}

static void os_sleep(time_t duration) {
    if (duration < 0) duration = 0;

    // Sleep in ms
    Sleep((u64)duration / 1000);
}

// ==== Random ====
// Get a random 64 bit number from the os
static u64 os_rand(void) {
    LARGE_INTEGER big_count;
    assert(QueryPerformanceCounter(&big_count));
    return (u64)big_count.QuadPart;
}

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
