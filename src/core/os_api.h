// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#pragma once
#include "core/type.h"

// The main function, to exit call os_exit()
void os_main(u32 argc, const char **argv);

// Allocate a new chunk of memory
// That is usually right after the previous allocation
static void *os_alloc(u32 size);
static void os_fail(const char *message) __attribute__((__noreturn__));
static void os_exit(u32 code) __attribute__((__noreturn__));

// File handling
typedef struct File File;
static File *os_open(const char *path);
static void os_close(File *file);
static u32 os_read(File *file, void *data, u32 size);
static u32 os_write(File *file, const void *data, u32 size);
static void os_seek(File *file, u32 pos);

// Dynamic library handling
typedef struct Library Library;
static Library *os_dlopen(const char *path);
static void *os_dlsym(Library *lib, const char *sym);

// Shell command
static i32 os_system(const char *command);

