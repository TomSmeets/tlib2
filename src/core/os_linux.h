// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#pragma once
#include "core/os_api.h"
#include "core/linux.h"
#include "core/type.h"
#include "core/str.h"

// Main function
int main(i32 argc, const char **argv) {
    for (;;) os_main(argc, argv);
}

// =================================
static void *os_alloc(u64 size) {
    void *ptr = linux_mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(ptr != MAP_FAILED);
    return ptr;
}

static void os_fail(const char *message) {
    linux_write(2, message, str_len(message));
    os_exit(1);
}

static void os_exit(i32 status) {
    linux_exit_group(status);
}

// =================================
static File *os_stdin(void) { return fd_to_ptr(0); }
static File *os_stdout(void) { return fd_to_ptr(1); }
static File *os_stderr(void) { return fd_to_ptr(2); }

static File *os_open(const char *path, File_Mode mode) {
    i32 flags = 0;
    u32 perm = 0;
    switch (mode) {
    case Open_Read:
        flags = O_RDONLY;
        break;
    case Open_Write:
        flags = O_WRONLY;
        perm = 0644;
        break;
    case Open_Create:
        flags = O_WRONLY | O_CREAT | O_TRUNC;
        perm = 0644;
        break;
    }

    i32 fd = linux_open(path, flags, perm);
    return fd_to_ptr(fd);
}

static void os_close(File *file) {
    assert(file);
    i32 ret = linux_close(ptr_to_fd(file));
    assert(ret == 0);
}

static i64 os_read(File *file, void *data, u64 size) {
    assert(file);
    return linux_read(ptr_to_fd(file), data, size);
}

static i64 os_write(File *file, const void *data, u64 size) {
    assert(file);
    return linux_write(ptr_to_fd(file), data, size);
}

static void os_seek(File *file, u64 pos) {
    assert(file);
    i64 result = linux_seek(ptr_to_fd(file), pos, SEEK_SET);
}

// =================================
static Library *os_dlopen(const char *path) {
    return (Library *)dlopen(path, RTLD_LOCAL | RTLD_NOW);
}

static void *os_dlsym(Library *lib, const char *sym) {
    return dlsym((void *)lib, sym);
}

static void *os_dlbase(Library *lib) {
    Dl_info info;
    void *addr = os_dlsym(lib, "os_main");
    dladdr(addr, &info);
    return info.fbase;
}

// ==== Time ====
static u64 os_time(void) {
    struct linux_timespec t = {};
    linux_clock_gettime(CLOCK_MONOTONIC, &t);
    return linux_timespec_to_us(&t);
}

static void os_sleep(u64 us) {
    struct linux_timespec time = linux_us_to_timespec(us);
    linux_nanosleep(&time, 0);
}

// ==== Random ====
static u64 os_rand(void) {
    u64 seed = 0;
    i64 ret = linux_getrandom(&seed, sizeof(seed), 0);
    assert_msg(ret == sizeof(seed), "linux getrandom failed");
    return seed;
}

// ==== System Commands ====
static i32 os_system(const char *command) {
    return system(command);
}
