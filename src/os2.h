// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// os2.h - Extra OS functions
#pragma once
#include "error.h"
#include "mem.h"
#include "os.h"

#if OS_LINUX
static char *os_cwd(Memory *mem) {
    char buffer[4 * 1024];
    try(linux_getcwd(buffer, sizeof(buffer)) > 0);
    size_t len = str_len(buffer);
    char *data = mem_alloc_uninit(mem, len + 1);
    mem_copy(data, buffer, len + 1);
    return data;
}
#endif

// Read entire file into memory
static bool os_read_file(Memory *mem, char *path, Buffer *result) {
    FileInfo info = {};
    try(os_stat(path, &info));

    File *fd = os_open(path, FileMode_Read);
    u8 *file_data = mem_array(mem, u8, info.size);
    u64 bytes_read = 0;
    try(os_read(fd, file_data, info.size, &bytes_read));
    try(bytes_read == info.size);
    try(os_close(fd));

    result->size = info.size;
    result->data = file_data;
    return ok();
}

static bool os_write_file(char *path, Buffer input) {
    File *fd = os_open(path, FileMode_Create);
    size_t bytes_written = 0;
    try(os_write(fd, input.data, input.size, &bytes_written));
    try(bytes_written == input.size);
    try(os_close(fd));
    return ok();
}
