// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_linux.h - Linux syscalls
#pragma once
#include "error.h"
#include "fs.h"
#include "linux.h"
#include "os_api.h"
#include "str.h"
#include "type.h"

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

// ==================================
//               Time
// ==================================

// ==== Random ====
// Get a random 64 bit number from the os
static u64 os_rand(void) {
    u64 seed = 0;
    i64 ret = linux_getrandom(&seed, sizeof(seed), 0);
    assert(ret == sizeof(seed));
    return seed;
}

// ==================================
//              Process
// ==================================

static void os_system(char *command) {
    check(system(command) == 0);
}

// Execute a system command, returns the exit code
static Process *os_exec(char **argv) {
    i32 pid = fork();

    // pid == 0 -> We are the child process
    if (pid == 0) {
        execvp(argv[0], argv);
        linux_exit_group(127);
    }

    // pid == -1 -> failed
    if (pid < 0) return 0;

    // pid > 0 -> parent process, pid is child PID
    return fd_to_handle(pid);
}

// Wait for process to exit and return exit code
static i32 os_wait(Process *proc) {
    i32 pid = fd_from_handle(proc);
    i32 status = 0;
    if (waitpid(pid, &status, 0) < 0) return -1;
    u32 sig = status & 0x7f;
    u32 exit_code = (status >> 8) & 0xff;
    if (sig) return -1;
    return exit_code;
}

// ==================================
//            File watcher
// ==================================

// Create a new file watches
static Watch *os_watch_new(void) {
    i32 fd = linux_inotify_init(O_NONBLOCK);
    if (fd < 0) return 0;
    return fd_to_handle(fd);
}

// Start watching a file or directory for changes
static bool os_watch_add(Watch *watch, char *path) {
    assert(watch);
    i32 fd = fd_from_handle(watch);
    i32 wd = linux_inotify_add_watch(fd, path, IN_MODIFY | IN_CREATE | IN_DELETE);
    return wd >= 0;
}

// Return true if a watched file was changed
static bool os_watch_check(Watch *watch) {
    i32 fd = fd_from_handle(watch);

    // Reserve enough space for a single event
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
