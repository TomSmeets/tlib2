// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_linux.h - Linux syscalls
#pragma once
#include "linux.h"
#include "os_api.h"
#include "str.h"
#include "type.h"

// The main function, to exit call os_exit()
// - This function is called in an infinite loop
// - not defined as static to support hot reloading
int main(i32 argc, char **argv) {
    for (;;) os_main(argc, argv);
}

// Allocate a new chunk of memory
// - returns null on failure
static void *os_alloc(size_t size) {
    void *ptr = linux_mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!ptr || ptr == MAP_FAILED) return 0;
    return ptr;
}

// Exit current application
// - Does not return
static void os_exit(i32 status) {
    linux_exit_group(status);
    __builtin_trap();
}

// Exit with an error message (error dialog)
// - Does not return
static void os_fail(char *message) {
    linux_write(2, message, str_len(message));
    os_exit(1);
}

// ==================================
//      File and Stream handling
// ==================================
static File *os_stdin(void) {
    return linux_file(0);
}

static File *os_stdout(void) {
    return linux_file(1);
}

static File *os_stderr(void) {
    return linux_file(2);
}

// Write data from file or stream
// - Returns actual number of bytes read in 'used' on success
// - Returns false on failure
static bool os_read(File *file, void *data, size_t size, size_t *used) {
    i64 ret = linux_read(linux_fd(file), data, size);
    if (ret < 0) return 0;
    if (used) *used = ret;
    return 1;
}

// Read data to file or stream
// - Returns actual number of bytes written in 'used' on success
// - Returns false on failure
static bool os_write(File *file, void *data, size_t size, size_t *used) {
    i64 ret = linux_write(linux_fd(file), data, size);
    if (ret < 0) return 0;
    if (used) *used = ret;
    return 1;
}

// Open a file for reading or writing
// - Returns 0 on failure
static File *os_open(char *path, FileMode mode) {
    assert(path);
    i32 ret = -1;
    if (mode == FileMode_Read) ret = linux_open(path, O_RDONLY, 0);
    if (mode == FileMode_Write) ret = linux_open(path, O_RDWR, 0);
    if (mode == FileMode_Create) ret = linux_open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (mode == FileMode_CreateExe) ret = linux_open(path, O_RDWR | O_CREAT | O_TRUNC, 0755);
    return linux_file(ret);
}

// Close a file
// - Returns false on failure
static bool os_close(File *file) {
    assert(file);
    return linux_close(linux_fd(file)) == 0;
}

// Seek to an absolute position in the current file
// - Returns false on failure
// - Files can be > 4GB
static bool os_seek(File *file, size_t pos) {
    assert(file);
    return linux_seek(linux_fd(file), pos, SEEK_SET) >= 0;
}

// Returns info in File_Info struct
// Returns false when the file does not exist
static bool os_stat(char *path, FileInfo *info) {
    assert(path);
    if (info) *info = (FileInfo){};

    struct linux_stat sb = {};
    i32 ret = linux_lstat(path, &sb);
    if (ret < 0) return false;

    if (info) {
        info->size = sb.st_size;
        info->mtime = time_from_ns(sb.st_mtime, sb.st_mtime_nsec);

        info->type = FileType_Other;
        u32 file_type = sb.st_mode & S_IFMT;
        if (file_type == S_IFREG) info->type = FileType_File;
        if (file_type == S_IFDIR) info->type = FileType_Directory;
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

// List directory contents
static bool os_list(char *path, os_list_cb *callback, void *user) {
    i32 dir = linux_open(path, O_RDONLY | O_DIRECTORY, 0);
    if (dir < 0) return 0;

    for (;;) {
        u8 buffer[4 * 1024];
        i64 len = linux_getdents64(dir, (void *)buffer, sizeof(buffer));

        // Error
        if (len < 0) {
            linux_close(dir);
            return 0;
        }

        // End of stream
        if (len == 0) break;

        // Read entries
        void *start = buffer;
        void *end = buffer + len;
        for (struct linux_dirent64 *ent = start; (void *)ent < end; ent = (void *)ent + ent->reclen) {
            // Skip '.' and '..'
            if (str_eq(ent->name, ".")) continue;
            if (str_eq(ent->name, "..")) continue;

            FileType type = FileType_Other;
            if (ent->type == DT_DIR) type = FileType_Directory;
            if (ent->type == DT_REG) type = FileType_File;

            if (!callback(user, ent->name, type)) {
                linux_close(dir);
                return 0;
            }
        }
    }

    linux_close(dir);
    return 1;
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

// ==================================
//               Time
// ==================================

// Get unix timestamp in micro seconds
static time_t os_time(void) {
    struct linux_timespec t = {};
    linux_clock_gettime(CLOCK_REALTIME, &t);
    return time_from_timespec(&t);
}

static void os_sleep(time_t duration) {
    if (duration < 0) duration = 0;
    struct linux_timespec time = time_to_timespec(duration);
    linux_nanosleep(&time, 0);
}

// ==== Random ====
// Get a random 64 bit number from the os
static u64 os_rand(void) {
    u64 seed = 0;
    i64 ret = linux_getrandom(&seed, sizeof(seed), 0);
    assert_msg(ret == sizeof(seed), "linux getrandom must succeed");
    return seed;
}

// ==================================
//              Process
// ==================================

static i32 os_system(char *command) {
    return system(command);
}

// Execute a system command, returns the exit code
static Process *os_exec(char **argv) {
    i32 pid = fork();

    // pid == 0 -> We are the child process
    if (pid == 0) {
        execvp(argv[0], argv);
        os_exit(127);
    }

    // pid == -1 -> failed
    if (pid < 0) return 0;

    // pid > 0 -> parent process, pid is child PID
    return linux_file(pid);
}

// Wait for process to exit and return exit code
static i32 os_wait(Process *proc) {
    i32 pid = linux_fd(proc);
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
