// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_api.h - Operating System API Interface
#pragma once
#include "type.h"

// The main function, to exit call os_exit()
// This function is called in an infinite loop
void os_main(u32 argc, char **argv);

// Allocate a new chunk of memory
static void *os_alloc(u64 size);

// Exit currnet application
static void os_exit(i32 status) __attribute__((__noreturn__));

// Exit with an error message (error dialog)
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
static i64 os_read(File *file, void *data, u64 size);

// Read data to file or stream
static i64 os_write(File *file, void *data, u64 size);

typedef enum {
    // Read only
    Open_Read,
    // Read and Write, no truncation
    Open_Write,
    // Create new file or truncate existing
    Open_Create,

    // Create new executable file
    Open_CreateExe,
} File_Mode;

// Open a file for reading or writing
static File *os_open(char *path, File_Mode mode);

// Close a file
static void os_close(File *file);

// Seek to an absolute position in the current file
static void os_seek(File *file, u64 pos);

// Get file size
static u64 os_file_size(File *file);

// Create an empty directory
static bool os_mkdir(char *path);

// Remove an empty directory
static bool os_rmdir(char *path);

// Remove a file
static bool os_remove(char *path);

// List contents of a directory
static u32 os_list(char *path, void *buffer, u64 size);

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

// Get current time in micro seconds
static u64 os_time(void);

// Sleep for a given time in micro seconds
static void os_sleep(u64 us);

// Get a random 64 bit number from the os
static u64 os_rand(void);

// Execute a system command, returns the exit code
static i32 os_system(char *command);

// Create a new file watches
static File *os_watch_new(void);

// Start watching a file or directory for changes
static void os_watch_add(File *watch, char *path);

// Return true if a watched file was changed
static bool os_watch_check(File *watch);
