// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// io.h - File IO
#pragma once
#include "buf.h"
#include "error.h"
#include "mem.h"
#include "os_headers.h"
#include "write.h"

// Abstract file handle
typedef struct File File;

// clang-format off
#if OS_LINUX || OS_WASM
static File *io_stdin(void)  { return fd_to_handle(0); }
static File *io_stdout(void) { return fd_to_handle(1); }
static File *io_stderr(void) { return fd_to_handle(2); }
#elif OS_WINDOWS
static File *io_stdin(void)  { return GetStdHandle(STD_INPUT_HANDLE);  }
static File *io_stdout(void) { return GetStdHandle(STD_OUTPUT_HANDLE); }
static File *io_stderr(void) { return GetStdHandle(STD_ERROR_HANDLE);  }
#endif
// clang-format on

// Close a file
static void io_close(File *file) {
#if OS_LINUX
    check(linux_close(fd_from_handle(file)) == 0);
#elif OS_WINDOWS
    check(CloseHandle(file));
#endif
}

// Seek to an absolute position in the current file
static void io_seek(File *file, size_t pos) {
#if OS_LINUX
    check(linux_seek(fd_from_handle(file), pos, SEEK_SET) >= 0);
#elif OS_WINDOWS
    LARGE_INTEGER offset;
    offset.QuadPart = pos;
    check(SetFilePointerEx(file, offset, 0, FILE_BEGIN));
#endif
}

// Try to fill buffer with data from the file
// - Returns actual number of bytes read
static size_t io_read_partial(File *file, Buffer buffer) {
#if OS_LINUX
    i64 ret = linux_read(fd_from_handle(file), buffer.data, buffer.size);
    check_or(ret >= 0) ret = 0;
    check_or(ret <= buffer.size) ret = buffer.size;
    return ret;
#elif OS_WINDOWS
    DWORD size = MIN(buffer.size, U32_MAX);
    DWORD used = 0;
    check(ReadFile(file, buffer.data, size, &used, 0));
    return used;
#else
    return 0;
#endif
}

// Try to write buffer data to the file
// - Returns actual number of bytes written
static size_t io_write_partial(File *file, Buffer buffer) {
#if OS_LINUX
    i64 ret = linux_write(fd_from_handle(file), buffer.data, buffer.size);
    check_or(ret >= 0) ret = 0;
    check_or(ret <= buffer.size) ret = buffer.size;
    return ret;
#elif OS_WINDOWS
    DWORD size = MIN(buffer.size, U32_MAX);
    DWORD used = 0;
    check(WriteFile(file, buffer.data, size, &used, 0));
    return used;
#elif OS_WASM
    if (file == io_stdout()) js_vpi("(str, len) => console.log(str_buf(str, len))", buffer.data, (i32)buffer.size);
    return buffer.size;
#else
    return 0;
#endif
}

// Fill buffer with data from the file
static void io_read(File *file, Buffer buffer) {
    while (buffer.size) {
        size_t inc = io_read_partial(file, buffer);
        check_or(inc > 0) break;
        buffer = buf_drop(buffer, inc);
    }
}

// Write buffer data to the file
static void io_write(File *file, Buffer buffer) {
    while (buffer.size) {
        size_t inc = io_write_partial(file, buffer);
        check_or(inc > 0) break;
        buffer = buf_drop(buffer, inc);
    }
}

static void *io_read_alloc(File *file, Memory *mem, size_t size) {
    void *ptr = mem_alloc_uninit(mem, size + 1);
    Buffer buffer = {ptr, size};
    io_read(file, buffer);
    return ptr;
}

static Buffer io_read_all_alloc(File *file, Memory *mem) {
    Write *write = write_new(mem);
    Buffer buffer = buf_stack(1024);
    while (1) {
        size_t used = io_read_partial(file, buffer);
        write_buffer(write, buf_take(buffer, used));
        if (used < buffer.size) break;
    }
    return write_get_written(write);
}
