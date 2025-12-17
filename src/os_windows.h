// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_linux.h - Linux syscalls
#pragma once
#include "os_api.h"
#include "str.h"
#include "type.h"
#include <windows.h>

// Main function
int main(i32 argc, char **argv) {
    for (;;) os_main(argc, argv);
}

// ==== File IO ======================================================
static void *os_alloc(u64 size) {
    return VirtualAlloc(
        // Let the system choose a starting address for us
        0,
        // Allocation size
        size,
        // Reserve the address range and commit the memory in that range
        MEM_COMMIT | MEM_RESERVE,
        // Memory should be read and writable
        PAGE_READWRITE
    );
}

static void os_exit(i32 status) {
    ExitProcess(status);
}

static void os_fail(char *message) {
    MessageBox(NULL, message, "Error", MB_ICONERROR | MB_OK);
    os_exit(1);
}

// =================================
static File *os_stdin(void) {
    return GetStdHandle(STD_INPUT_HANDLE);
}

static File *os_stdout(void) {
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

static File *os_stderr(void) {
    return GetStdHandle(STD_ERROR_HANDLE);
}

static bool os_read(File *file, void *data, u64 size, u64 *used) {
    if (used) *used = 0;
    while (size > 0) {
        u32 used_small = 0;
        bool ret = ReadFile(file, data, MIN(size, U32_MAX), &used_small, NULL);
        if (!ret) return false;
        if (used_small == 0) break;
        data += used_small;
        size -= used_small;
        if (used) *used += used_small;
    }
    return true;
}

static bool os_write(File *file, void *data, u64 size, u64 *used) {
    if (used) *used = 0;
    while (size > 0) {
        u32 used_small = 0;
        bool ret = WriteFile(file, data, MIN(size, U32_MAX), &used_small, NULL);
        if (!ret) return false;
        if (used_small == 0) break;
        data += used_small;
        size -= used_small;
        if (used) *used += used_small;
    }
    return true;
}

static File *os_open(char *path, File_Mode mode) {
    assert(path);
    HANDLE ret = 0;
    if (mode == Open_Read) ret = CreateFile(path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (mode == Open_Write) ret = CreateFile(path, GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (mode == Open_Create) ret = CreateFile(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (mode == Open_CreateExe) ret = CreateFile(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (ret == INVALID_HANDLE_VALUE) return 0;
    return (File *)ret;
}

static bool os_close(File *file) {
    assert(file);
    return CloseHandle(file);
}

static bool os_seek(File *file, u64 pos) {
    assert(file);
    LARGE_INTEGER offset;
    offset.QuadPart = pos; // 1 GB
    return SetFilePointerEx(file, offset, 0, FILE_BEGIN);
}

static bool os_stat(char *path, File_Info *info) {
    if(info) *info = (File_Info){};
    WIN32_FILE_ATTRIBUTE_DATA winfo;
    bool ret = GetFileAttributesExA(path, GetFileExInfoStandard, &winfo);
    if(!ret) return false;
    if (info) {
        bool is_dir = info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        info->size = ((u64)info.nFileSizeHigh << 32) | (u64)info.nFileSizeLow;
        // TODO: info->mtime;
        info->is_file = !is_dir;
        info->is_dir = is_dir;
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
    return (Library *)LoadLibrary(path);
}

// Lookup a symbol in a library
static void *os_dlsym(Library *lib, char *sym) {
    return GetProcAddress((void *)lib, sym);
}

// Get base address of a library based on a pointer inside that library
static void *os_dlbase(void *ptr) {
    // Not supported
    return 0;
}

// ==== Time ====
static u64 os_time(void) {
    LARGE_INTEGER big_freq, big_count;
    assert(QueryPerformanceFrequency(&big_freq), "Failed to get performance frequency");
    assert(QueryPerformanceCounter(&big_count), "Failed to get performance counter");
    i64 freq = big_freq.QuadPart;
    i64 count = big_count.QuadPart;
    assert_msg(freq >= 1000 * 1000, "Invalid performance frequency");
    assert_msg(count >= 0, "Invalid performance counter");
    return (u64)count / ((u64)freq / 1000 / 1000);
}

static void os_sleep(u64 us) {
    // Sleep in ms
    Sleep(time / 1000);
}

// ==== Random ====
// Get a random 64 bit number from the os
static u64 os_rand(void) {
    return os_time();
}

// ==== System Commands ====
static i32 os_system(char *command) {
    return system(command);
}

// Execute a system command, returns the exit code
static i32 os_exec(u32 argc, char **argv) {
    return -1;
}

static Watch *os_watch_new(void) {
    // Not supported
    return 0;
}

static bool os_watch_add(Watch *watch, char *path) {
    // Not supported
    return false;
}

static bool os_watch_check(Watch *watch) {
    // Not supported
    return 0;
}

