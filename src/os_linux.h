// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_linux.h - Linux syscalls
#pragma once
#include "linux.h"
#include "os_api.h"
#include "str.h"
#include "type.h"

// Main function
int main(i32 argc, char **argv) {
    for (;;) os_main(argc, argv);
}

// ==== File IO ======================================================
static void *os_alloc(u64 size) {
    void *ptr = linux_mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(ptr != MAP_FAILED);
    return ptr;
}

static void os_fail(char *message) {
    linux_write(2, message, str_len(message));
    os_exit(1);
}

static void os_exit(i32 status) {
    linux_exit_group(status);
}

// =================================
static File *os_stdin(void) {
    return fd_to_ptr(0);
}
static File *os_stdout(void) {
    return fd_to_ptr(1);
}
static File *os_stderr(void) {
    return fd_to_ptr(2);
}

static i64 os_read(File *file, void *data, u64 size) {
    assert(file);
    return linux_read(ptr_to_fd(file), data, size);
}

static i64 os_write(File *file, void *data, u64 size) {
    assert(file);
    return linux_write(ptr_to_fd(file), data, size);
}

static File *os_open(char *path, File_Mode mode) {
    i32 flags = 0;
    u32 perm = 0644;
    if (mode == Open_Read) flags |= O_RDONLY;
    if (mode == Open_Write) flags |= O_RDONLY | O_WRONLY;
    if (mode == Open_Create || mode == Open_CreateExe) flags |= O_RDONLY | O_WRONLY | O_CREAT | O_TRUNC;
    if (mode == Open_CreateExe) perm = 0755;
    i32 fd = linux_open(path, flags, perm);
    return fd_to_ptr(fd);
}

static void os_close(File *file) {
    assert(file);
    i32 ret = linux_close(ptr_to_fd(file));
    assert(ret == 0);
}

static void os_seek(File *file, u64 pos) {
    assert(file);
    i64 result = linux_seek(ptr_to_fd(file), pos, SEEK_SET);
}

static u64 os_file_size(File *file) {
    struct linux_stat sb = {};
    linux_fstat(ptr_to_fd(file), &sb);
    if (sb.st_size < 0) return 0;
    return sb.st_size;
}

// =================================
static Library *os_dlopen(char *path) {
    return (Library *)dlopen(path, RTLD_LOCAL | RTLD_NOW);
}

static void *os_dlsym(Library *lib, char *sym) {
    return dlsym((void *)lib, sym);
}

static void *os_dlbase(void *ptr) {
    Dl_info info;
    dladdr(ptr, &info);
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
static i32 os_system(char *command) {
    return system(command);
}

// Create a new file watches
static File *os_watch_new(void) {
    i64 fd = linux_inotify_init(O_NONBLOCK);
    assert(fd >= 0);
    return fd_to_ptr(fd);
}

// Start watching a file or directory for changes
static void os_watch_add(File *watch, char *path) {
    i64 fd = ptr_to_fd(watch);
    i32 wd = linux_inotify_add_watch(fd, path, IN_MODIFY | IN_CREATE | IN_DELETE);
    assert(wd >= 0);
}

// Return true if a watched file was changed
static bool os_watch_check(File *watch) {
    i64 fd = ptr_to_fd(watch);
    u8 buffer[sizeof(struct inotify_event) + NAME_MAX + 1];

    for (;;) {
        i64 length = linux_read(fd, buffer, sizeof(buffer));

        // No more data
        if (length == -EAGAIN) return false;

        // Some other error
        assert(length >= 0);

        // Change!
        struct inotify_event *event = (struct inotify_event *)buffer;
        return true;
    }
}
