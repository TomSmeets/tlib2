// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// time.h - Timing and sleeps
#pragma once
#include "os_headers.h"
#include "type.h"

// Unix time in micro seconds
typedef i64 time_t;

// One Millisecond
#define TIME_MS ((time_t)1000)

// One Second
#define TIME_SEC ((time_t)1000000)

// Get time micro seconds since 01-01-1970
static time_t time_now(void) {
#if OS_LINUX
    struct linux_timespec t = {};
    linux_clock_gettime(CLOCK_REALTIME, &t);
    return time_from_timespec(&t);
#elif OS_WINDOWS
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
#elif OS_WASM
    return wasm_call_l("() => BigInt(new Date().getTime()*1000)");
#endif
}

static void os_sleep(time_t duration) {
    if (duration < 0) duration = 0;
#if OS_LINUX
    struct linux_timespec time = time_to_timespec(duration);
    linux_nanosleep(&time, 0);
#elif OS_WINDOWS
    // Sleep in ms
    Sleep((u64)duration / 1000);
#elif OS_WASM
    // Actual sleep is not possible in js :(
    wasm_call_vl("(t) => tlib.next_sleep = t", duration);
#endif
}
