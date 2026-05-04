// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_api.h - Operating System API Interface
#pragma once
#include "buf.h"
#include "fs.h"
#include "type.h"

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
static void os_system(char *command);

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
