// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#pragma once
#include "core/type.h"

// The main function, to exit call os_exit()
void os_main(u32 argc, const char **argv);

// Allocate a new chunk of memory
static void *os_alloc(u64 size);
static void os_fail(const char *message) __attribute__((__noreturn__));
static void os_exit(i32 status) __attribute__((__noreturn__));

// File handling
typedef struct File File;
typedef enum {
    Open_Read,
    Open_Write,
    Open_Create,
} File_Mode;

static File *os_open(const char *path, File_Mode mode);
static void os_close(File *file);
static u64 os_read(File *file, void *data, u64 size);
static u64 os_write(File *file, const void *data, u64 size);
static void os_seek(File *file, u64 pos);

// Dynamic library handling
typedef struct Library Library;

// Open a library by name or full path
static Library *os_dlopen(const char *path);

// Lookup a symbol in a library
static void *os_dlsym(Library *lib, const char *sym);

// Get base pointer of a library
static void *os_dlbase(Library *lib);

// Get current time in micro seconds
static u64 os_time(void);

// Sleep for a given time in micro seconds
static void os_sleep(u64 us);

// Get a random 64 bit number from the os
static u64 os_rand(void);

// Execute a system command, returns the exit code
static i32 os_system(const char *command);
