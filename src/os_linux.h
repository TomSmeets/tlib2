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
    if (!ptr || ptr == MAP_FAILED) return 0;
    return ptr;
}

static void os_exit(i32 status) {
    linux_exit_group(status);
}

static void os_fail(char *message) {
    linux_write(2, message, str_len(message));
    os_exit(1);
}


// =================================
static File *os_stdin(void) {
    return linux_file(0);
}

static File *os_stdout(void) {
    return linux_file(1);
}

static File *os_stderr(void) {
    return linux_file(2);
}

static bool os_read(File *file, void *data, u64 size, u64 *used) {
    i64 ret = linux_read(linux_fd(file), data, size);
    if (used) *used = MAX(ret, 0);
    return ret >= 0;
}

static bool os_write(File *file, void *data, u64 size, u64 *used) {
    i64 ret = linux_write(linux_fd(file), data, size);
    if (used) *used = MAX(ret, 0);
    return ret >= 0;
}

static File *os_open(char *path, File_Mode mode) {
    assert(path);
    i32 ret = -1;
    if (mode == Open_Read) ret = linux_open(path, O_RDONLY, 0);
    if (mode == Open_Write) ret = linux_open(path, O_RDWR, 0);
    if (mode == Open_Create) ret = linux_open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (mode == Open_CreateExe) ret = linux_open(path, O_RDWR | O_CREAT | O_TRUNC, 0755);
    return linux_file(ret);
}

static bool os_close(File *file) {
    assert(file);
    i32 ret = linux_close(linux_fd(file));
    return ret == 0;
}

static bool os_seek(File *file, u64 pos) {
    assert(file);
    return linux_seek(linux_fd(file), pos, SEEK_SET) >= 0;
}

static bool os_stat(char *path, File_Info *info) {
    if(info) *info = (File_Info){};

    struct linux_stat sb = {};
    i32 ret = linux_lstat(path, &sb);
    if(ret < 0) return false;

    if (info) {
        info->size = sb.st_size;
        info->mtime = time_from_ns(sb.st_mtime, sb.st_mtime_nsec);

        u32 file_type = sb.st_mode & S_IFMT;
        info->is_file = file_type == S_IFREG;
        info->is_dir = file_type == S_IFDIR;
    }
    return true;
}

// Create an empty directory
static bool os_mkdir(char *path) {
    return linux_mkdir(path, 0755) == 0;
}

// Remove an empty directory
static bool os_rmdir(char *path) {
    return linux_rmdir(path) == 0;
}

// Remove a file
static bool os_remove(char *path) {
    return linux_unlink(path) == 0;
}

// =================================

// Open a library by name or full path
static Library *os_dlopen(char *path) {
    return (Library *)dlopen(path, RTLD_LOCAL | RTLD_NOW);
}

// Lookup a symbol in a library
static void *os_dlsym(Library *lib, char *sym) {
    return dlsym((void *)lib, sym);
}

// Get base address of a library based on a pointer inside that library
static void *os_dlbase(void *ptr) {
    Dl_info info;
    int ret = dladdr(ptr, &info);
    // Returns 0 on failure
    if (ret == 0) return 0;
    return info.fbase;
}

// ==== Time ====
static u64 os_time(void) {
    struct linux_timespec t = {};
    linux_clock_gettime(CLOCK_MONOTONIC, &t);
    return time_from_timespec(&t);
}

static void os_sleep(u64 us) {
    struct linux_timespec time = time_to_timespec(us);
    linux_nanosleep(&time, 0);
}

// ==== Random ====
// Get a random 64 bit number from the os
static u64 os_rand(void) {
    u64 seed = 0;
    i64 ret = linux_getrandom(&seed, sizeof(seed), 0);
    // Must suceed!
    assert_msg(ret == sizeof(seed), "linux getrandom failed");
    return seed;
}

// ==== System Commands ====
static i32 os_system(char *command) {
    return system(command);
}

// Execute a system command, returns the exit code
static i32 os_exec(u32 argc, char **argv) {
    return -1;
}

// Create a new file watches
static Watch *os_watch_new(void) {
    i32 fd = linux_inotify_init(O_NONBLOCK);
    if (fd < 0) return 0;
    return linux_file(fd);
}

// Start watching a file or directory for changes
static bool os_watch_add(Watch *watch, char *path) {
    assert(watch);
    i32 fd = linux_fd(watch);
    i32 wd = linux_inotify_add_watch(fd, path, IN_MODIFY | IN_CREATE | IN_DELETE);
    return wd >= 0;
}

// Return true if a watched file was changed
static bool os_watch_check(Watch *watch) {
    i32 fd = linux_fd(watch);

    // Reserve enogh space for a single event
    u8 buffer[sizeof(struct inotify_event) + NAME_MAX + 1];

    bool status = 0;
    while (1) {
        i64 ret = linux_read(fd, buffer, sizeof(buffer));

        // No more data
        if (ret == -EAGAIN) return status;

        // Some other error, should not happen
        assert(ret >= 0);

        // Something changed!
        struct inotify_event *event = (struct inotify_event *)buffer;

        // Don't care about the event details
        (void)event;

        // Keep reading, to clear the buffer
        status = 1;
    }
}
