// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// os2.h - Extra OS functions
#pragma once
#include "error.h"
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

// Read entire file into memory
static Buffer os_read_file(Memory *mem, char *path) {
    FileInfo info = {};
    check(os_stat(path, &info));
    if (error) return buf_null();

    File *fd = os_open(path, FileMode_Read);
    u8 *file_data = mem_array(mem, u8, info.size + 1);
    file_data[info.size] = 0;
    u64 bytes_read = 0;
    check(os_read(fd, file_data, info.size, &bytes_read));
    check(bytes_read == info.size);
    check(os_close(fd));
    return buf_from(file_data, info.size);
}

static char *os_read_file_string(Memory *mem, char *path) {
    return (char *)os_read_file(mem, path).data;
}

static void os_write_file(char *path, Buffer input) {
    File *fd = os_open(path, FileMode_Create);
    size_t bytes_written = 0;
    check(os_write(fd, input.data, input.size, &bytes_written));
    check(bytes_written == input.size);
    check(os_close(fd));
}
