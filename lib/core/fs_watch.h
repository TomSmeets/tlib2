#pragma once
#include "error.h"
#include "fs.h"
#include "io.h"
#include "os_headers.h"

#if OS_LINUX
#define IN_MODIFY 0x00000002
#define IN_CREATE 0x00000100
#define IN_DELETE 0x00000200

struct inotify_event {
    i32 wd;
    u32 mask;
    u32 cookie;
    u32 len;
    char name[];
};

static long linux_inotify_init1(int flags) {
    return linux_syscall1(294, flags);
}

static long linux_inotify_add_watch(int fd, const char *path, u32 mask) {
    return linux_syscall3(254, fd, (i64)path, mask);
}
#endif

typedef struct Watch Watch;

// Create a new file watches
static Watch *fs_watch_new(void) {
    Watch *ret = 0;

    IF_LINUX({
        i32 fd = linux_inotify_init1(O_NONBLOCK);
        check_or(fd >= 0) return 0;
        ret = fd_to_handle(fd);
    })

    return ret;
}

// Start watching a file or directory for changes
static void os_watch_add(Watch *watch, char *path) {
    IF_LINUX({
        i32 fd = fd_from_handle(watch);
        i32 wd = linux_inotify_add_watch(fd, path, IN_MODIFY | IN_CREATE | IN_DELETE);
        check(wd >= 0);
    })
}

// Return true if a watched file was changed
static bool os_watch_check(Watch *watch) {
    IF_LINUX({
        i32 fd = fd_from_handle(watch);

        // Reserve enough space for a single event
        char buffer[sizeof(struct inotify_event) + NAME_MAX + 1];

        bool status = 0;
        while (1) {
            i64 ret = sys_read(fd, buffer, sizeof(buffer));

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
    })
    return false;
}
