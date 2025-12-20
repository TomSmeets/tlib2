// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_api.h - Operating System API Interface
#pragma once
#include "type.h"

// The main function, to exit call os_exit()
// - This function is called in an infinite loop
// - not defined as static to support hot reloading
void os_main(u32 argc, char **argv);

// Allocate a new chunk of memory
// - returns null on failure
static void *os_alloc(size_t size);

// Exit currnet application
// - Does not return
static void os_exit(i32 status) __attribute__((__noreturn__));

// Exit with an error message (error dialog)
// - Does not return
static void os_fail(char *message) __attribute__((__noreturn__));

// ==================================
//      File and Stream handling
// ==================================
typedef struct File File;

// Standard file handles
static File *os_stdin(void);
static File *os_stdout(void);
static File *os_stderr(void);

// Write data from file or stream
// - Returns actual number of bytes read in 'used' on success
// - Returns false on failure
static bool os_read(File *file, void *data, size_t size, size_t *used);

// Read data to file or stream
// - Returns actual number of bytes written in 'used' on success
// - Returns false on failure
static bool os_write(File *file, void *data, size_t size, size_t *used);

typedef enum {
    // Read only
    FileMode_Read,
    // Read and Write, no truncation
    FileMode_Write,
    // Create new file or truncate existing
    FileMode_Create,
    // Create new executable file
    FileMode_CreateExe,
} FileMode;

// Open a file for reading or writing
// - Returns 0 on failure
static File *os_open(char *path, FileMode mode);

// Close a file
// - Returns false on failure
static bool os_close(File *file);

// Seek to an absolute position in the current file
// - Returns false on failure
// - Files can be > 4GB
static bool os_seek(File *file, size_t pos);

typedef enum {
    FileType_None,
    FileType_File,
    FileType_Directory,
    FileType_Other,
} FileType;

// Get file size
typedef struct {
    size_t size;
    time_t mtime;
    FileType type;
} FileInfo;

// Returns info in FileInfo struct
// Returns false when the file does not exist
static bool os_stat(char *path, FileInfo *info);

// Create an empty directory
static bool os_mkdir(char *path);

// Remove an empty directory
static bool os_rmdir(char *path);

// Remove a file
static bool os_remove(char *path);

typedef bool os_list_cb(void *user, char *name, FileType type);

// List directory contents
static bool os_list(char *path, os_list_cb *callback, void *user);

// ==================================
//      Dynamic library handling
// ==================================
typedef struct Library Library;

// Open a library by name or full path
static Library *os_dlopen(char *path);

// Lookup a symbol in a library
static void *os_dlsym(Library *lib, char *sym);

// Get base address of a library based on a pointer inside that library
static void *os_dlbase(void *ptr);

// ==================================
//               Time
// ==================================

// Get unix timestamp in micro seconds
static time_t os_time(void);

// Sleep for a given time in micro seconds
static void os_sleep(time_t us);

// Get a random 64 bit number from the os
static u64 os_rand(void);

// ==================================
//              Process
// ==================================
typedef struct Process Process;

// Execute a shell command, returns the exit code
static i32 os_system(char *command);

// Execute a process, returns the exit code
// - argv is a null terminated list of strings
static Process *os_exec(char **argv);

// Wait for process to exit and return exit code
static i32 os_wait(Process *proc);

// ==================================
//            File watcher
// ==================================
typedef struct Watch Watch;

// Create a new file watcher
// - Returns 0 on failure or when not supported
static Watch *os_watch_new(void);

// Start watching a directory for changes
// - Returns 0 on failure
static bool os_watch_add(Watch *watch, char *path);

// Check directories for changes without blocking
// - Returns 1 if one or more files changed since last check
static bool os_watch_check(Watch *watch);
