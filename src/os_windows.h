// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_linux.h - Linux syscalls
#pragma once
#include "os_api.h"
#include "str.h"
#include "type.h"
#include <windows.h>

// The main function, to exit call os_exit()
// - This function is called in an infinite loop
// - not defined as static to support hot reloading
int main(int argc, char **argv) {
    for (;;) os_main(argc, argv);
}

// Allocate a new chunk of memory
// - returns null on failure
static void *os_alloc(size_t size) {
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

// Exit currnet application
// - Does not return
static void os_exit(i32 status) {
    ExitProcess(status);
}

// Exit with an error message (error dialog)
// - Does not return
static void os_fail(char *message) {
    MessageBoxA(NULL, message, "Error", MB_ICONERROR | MB_OK);
    os_exit(1);
}

// ==================================
//      File and Stream handling
// ==================================
static File *os_stdin(void) {
    return GetStdHandle(STD_INPUT_HANDLE);
}

static File *os_stdout(void) {
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

static File *os_stderr(void) {
    return GetStdHandle(STD_ERROR_HANDLE);
}

// Write data from file or stream
// - Returns actual number of bytes read in 'used' on success
// - Returns false on failure
static bool os_read(File *file, void *data, size_t size, size_t *used) {
    return ReadFile(file, data, size, used, 0);
}

// Read data to file or stream
// - Returns actual number of bytes written in 'used' on success
// - Returns false on failure
static bool os_write(File *file, void *data, size_t size, size_t *used) {
    return WriteFile(file, data, size, used, 0);
}


// Open a file for reading or writing
// - Returns 0 on failure
static File *os_open(char *path, File_Mode mode) {
    assert(path);
    HANDLE ret = 0;
    if (mode == Open_Read) ret = CreateFileA(path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (mode == Open_Write) ret = CreateFileA(path, GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (mode == Open_Create) ret = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (mode == Open_CreateExe) ret = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (ret == INVALID_HANDLE_VALUE) return 0;
    return (File *)ret;
}

// Close a file
// - Returns false on failure
static bool os_close(File *file) {
    assert(file);
    return CloseHandle(file);
}

// Seek to an absolute position in the current file
// - Returns false on failure
// - Files can be > 4GB
static bool os_seek(File *file, size_t pos) {
    assert(file);
    LARGE_INTEGER offset;
    offset.QuadPart = pos;
    return SetFilePointerEx(file, offset, 0, FILE_BEGIN);
}

// Returns info in File_Info struct
// Returns false when the file does not exist
static bool os_stat(char *path, File_Info *info) {
    if (info) *info = (File_Info){};
    WIN32_FILE_ATTRIBUTE_DATA winfo;
    bool ret = GetFileAttributesExA(path, GetFileExInfoStandard, &winfo);
    if (!ret) return false;
    if (info) {
        bool is_dir = winfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        info->size = ((size_t)winfo.nFileSizeHigh << 32) | (size_t)winfo.nFileSizeLow;
        // TODO: info->mtime;
        info->is_file = !is_dir;
        info->is_dir = is_dir;
    }
    return true;
}

// Create an empty directory
static bool os_mkdir(char *path) {
    return CreateDirectoryA(path, NULL);
}

// Remove an empty directory
static bool os_rmdir(char *path) {
    return RemoveDirectoryA(path);
}

// Remove a file
static bool os_remove(char *path) {
    return DeleteFileA(path) == 0;
}

// List directory contents
static bool os_list(char *path, os_list_cb *callback, void *user) {
    // Construct a query: path + '\*'
    size_t path_len = str_len(path);
    char search_query[path_len + 3];
    std_memcpy(search_query, path, path_len);
    std_memcpy(search_query + path_len, "\\*", 2);
    search_query[path_len + 3] = 0;

    WIN32_FIND_DATAA find;
    HANDLE handle = FindFirstFileA(search, &find);
    if (handle == INVALID_HANDLE_VALUE) return 0;

    do {
        char *name = find.cFileName;

        // Skip "." and ".."
        if (str_eq(name, ".")) continue;
        if (str_eq(name, "..")) continue;

        FileType type = FileType_File;
        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            type = FileType_Directory;

        if (!callback(user, name, type)) {
            FindClose(handle);
            return 0;
        }
    } while (FindNextFileA(handle, &find));

    FindClose(handle);
    return 1;
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
static i64 os_time(void) {
    static u64 freq;
    static i64 offset;

    // Get perf counter
    LARGE_INTEGER big_count;
    assert(QueryPerformanceCounter(&big_count));
    u64 count = (u64)big_count.QuadPart;

    if(!freq) {
        // Get counter frequency
        LARGE_INTEGER big_freq;
        assert(QueryPerformanceFrequency(&big_freq));
        freq = (u64)big_freq.QuadPart / 1000 / 1000;
        assert_msg(freq > 0, "Invalid performance frequency");
        
        // Initialize offset to match system time
        FILETIME ft;
        assert(GetSystemTimePreciseAsFileTime(&ft));
        u64 win_time  = ((u64)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
        i64 unix_time = (i64)(time / 10) - 11644473600000000LL;
        offset = unix_time - (i64)(count / freq);
    }

    return (i64)(count / freq) + offset;
}

static void os_sleep(i64 us) {
    if (us < 0) us = 0;

    // Sleep in ms
    Sleep((u64)us / 1000);
}

// ==== Random ====
// Get a random 64 bit number from the os
static u64 os_rand(void) {
    LARGE_INTEGER big_count;
    assert(QueryPerformanceCounter(&big_count));
    return (u64)big_count.QuadPart;
}

// ==== System Commands ====
typedef struct {
    File *stdin;
    File *stdout;
    File *stderr;
    void *handle;
} Process;

static i32 os_system(char *command) {
    return system(command);
}

// Execute a system command, returns the exit code
// - First argument is the process to execute
static i32 os_spawn(u32 argc, char **argv) {
    return -1;
}

// ==================================
//            File watcher
// ==================================
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
