// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_wasm.h - WASM OS abstraction
#pragma once
#include "os_api.h"
#include "str.h"
#include "type.h"

#define WASM_IMPORT(name) __attribute((import_module("env"), import_name(#name)))

static void *os_alloc(size_t size) {
    size_t page_size = 65536;
    size_t pages = (size + page_size - 1) / page_size;
    size_t addr = __builtin_wasm_memory_grow(0, pages) * page_size;
    return (void *)addr;
}

WASM_IMPORT(wasm_write) void wasm_exit(void);
static void os_exit(i32 status) {
    wasm_exit();
    __builtin_trap();
}

WASM_IMPORT(wasm_fail) void wasm_fail(char *message, u32 len);
static void os_fail(char *message) {
    wasm_fail(message, str_len(message));
    __builtin_trap();
}

static File *wasm_file(u32 fd) {
    return (File *)(intptr_t)fd;
}

static u32 wasm_fd(File *file) {
    return (u32)(intptr_t)file;
}

// ==================================
//      File and Stream handling
// ==================================
// clang-format off
static File *os_stdin(void)  { return wasm_file(1); }
static File *os_stdout(void) { return wasm_file(2); }
static File *os_stderr(void) { return wasm_file(3); }
// clang-format on

static bool os_read(File *file, void *data, size_t size, size_t *used) {
    os_fail("Not implemented");
}

WASM_IMPORT(wasm_write) bool wasm_write(u32 fd, void *data, size_t size);
static bool os_write(File *file, void *data, size_t size, size_t *used) {
    assert(file);
    if (used) *used = size;
    return wasm_write(wasm_fd(file), data, size);
}

static File *os_open(char *path, FileMode mode) {
    os_fail("Not implemented");
}

static bool os_close(File *file) {
    os_fail("Not implemented");
}

static bool os_seek(File *file, size_t pos) {
    os_fail("Not implemented");
}

static bool os_stat(char *path, FileInfo *info) {
    os_fail("Not implemented");
}

// Create an empty directory
static bool os_mkdir(char *path) {
    os_fail("Not implemented");
}

// Remove an empty directory
static bool os_rmdir(char *path) {
    os_fail("Not implemented");
}

// Remove a file
static bool os_remove(char *path) {
    os_fail("Not implemented");
}

// List directory contents
static bool os_list(char *path, os_list_cb *callback, void *user) {
    os_fail("Not implemented");
}

// =================================

// Open a library by name or full path
static Library *os_dlopen(char *path) {
    os_fail("Not implemented");
}

// Lookup a symbol in a library
static void *os_dlsym(Library *lib, char *sym) {
    os_fail("Not implemented");
}

// Get base address of a library based on a pointer inside that library
static void *os_dlbase(void *ptr) {
    os_fail("Not implemented");
}

WASM_IMPORT(wasm_time) time_t wasm_time(void);
static time_t os_time(void) {
    return wasm_time();
}

WASM_IMPORT(wasm_sleep) void wasm_sleep(time_t duration);
static void os_sleep(time_t duration) {
    if (duration < 0) duration = 0;
    wasm_sleep(duration);
}

static u64 os_rand(void) {
    return os_time();
}

WASM_IMPORT(wasm_eval) i32 wasm_eval(char *code);
static i32 os_system(char *command) {
    return wasm_eval(command);
}

static Process *os_exec(char **argv) {
    os_fail("Not implemented");
}

static i32 os_wait(Process *proc) {
    os_fail("Not implemented");
}

static Watch *os_watch_new(void) {
    os_fail("Not implemented");
}

static bool os_watch_add(Watch *watch, char *path) {
    os_fail("Not implemented");
}

static bool os_watch_check(Watch *watch) {
    os_fail("Not implemented");
}
