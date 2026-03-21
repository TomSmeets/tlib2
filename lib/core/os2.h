// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// os2.h - Extra OS functions
#pragma once
#include "error.h"
#include "fmt.h"
#include "mem.h"
#include "os.h"

#if OS_LINUX
static char *os_cwd(Memory *mem) {
    char buffer[4 * 1024];
    check(linux_getcwd(buffer, sizeof(buffer)) > 0);
    if (error) return 0;

    size_t len = str_len(buffer);
    char *data = mem_alloc_uninit(mem, len + 1);
    mem_copy(data, buffer, len + 1);
    return data;
}
#endif

static void os_read_exact(File *file, Buffer buf) {
    while (buf.size) {
        size_t read = os_read(file, buf);
        buf = buf_drop(buf, read);
        if (error) break;
    }
}

static void os_write_exact(File *file, Buffer buf) {
    while (buf.size) {
        size_t written = os_write(file, buf);
        buf = buf_drop(buf, written);
        if (error) break;
    }
}

// Read entire file into memory
static Buffer os_read_file(Memory *mem, char *path) {
    FileInfo info = {};
    check(os_stat(path, &info));
    if (error) return buf_null();

    File *fd = os_open(path, FileMode_Read);

    u8 *file_data = mem_array(mem, u8, info.size + 1);
    file_data[info.size] = 0;
    os_read_exact(fd, buf_from(file_data, info.size));
    os_close(fd);
    return buf_from(file_data, info.size);
}

// Read 'size' bytes from file
static void *os_read_alloc(Memory *mem, File *file, u32 size) {
    Buffer buffer = mem_buffer(mem, size);
    os_read_exact(file, buffer);
    return buffer.data;
}

static char *os_read_file_string(Memory *mem, char *path) {
    return (char *)os_read_file(mem, path).data;
}

static void os_write_file(char *path, Buffer input) {
    File *fd = os_open(path, FileMode_Create);
    os_write_exact(fd, input);
    os_close(fd);
}

static void os_file_copy(char *src_path, char *dst_path) {
    File *src = os_open(src_path, FileMode_Read);
    File *dst = os_open(dst_path, FileMode_CreateExe);

    Buffer buffer = buf_stack(4 * 1024);
    for (;;) {
        u64 bytes_read = os_read(src, buffer);
        if (error) break;
        if (bytes_read == 0) break;
        os_write(dst, buf_take(buffer, bytes_read));
        if (error) break;
    }
    os_close(src);
    os_close(dst);
}
