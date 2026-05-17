// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// fs.h - Filesystem Access
#pragma once
#include "buf.h"
#include "error.h"
#include "io.h"
#include "mem.h"
#include "os_headers.h"
#include "ptr.h"
#include "str.h"
#include "time.h"

// Maximum path length
#define PATH_MAX (4 * 1024)

// File open mode
typedef enum {
    // Read only
    FileMode_Read,
    // Read and Write, no truncation
    FileMode_Write,
    // Create new file or truncate existing
    FileMode_Create,
    // Create new executable file
    FileMode_CreateExe,
} FileMode;

static void buf_split_backwards(Buffer buf, u8 key, Buffer *left, Buffer *right) {
    for (i32 i = buf.size - 1; i >= 0; --i) {
        if (buf.data[i] == key) {
            *left = buf_take(buf, i);
            *right = buf_drop(buf, i + 1);
            return;
        }
    }
    *left = buf;
    *right = (Buffer){};
}

typedef struct {
    Buffer parent;
    Buffer file;
    Buffer base;
    Buffer ext;
} Path_Components;

static Path_Components path_split(Buffer path) {
    Path_Components ret = {};
    // Find last '/'
    buf_split_backwards(path, '/', &ret.parent, &ret.file);
    buf_split_backwards(ret.file, '.', &ret.base, &ret.ext);
    return ret;
}

#if OS_LINUX
#define NAME_MAX 255
static long sys_open(const char *filename, int flags, umode_t mode) {
    return linux_syscall3(2, (i64)filename, flags, mode);
}
#endif

// Open a file for reading or writing
static File *fs_open(char *path, FileMode mode) {
    IF_LINUX({
        i32 ret = -1;
        if (mode == FileMode_Read) ret = sys_open(path, O_RDONLY, 0);
        if (mode == FileMode_Write) ret = sys_open(path, O_RDWR, 0);
        if (mode == FileMode_Create) ret = sys_open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (mode == FileMode_CreateExe) ret = sys_open(path, O_RDWR | O_CREAT | O_TRUNC, 0755);
        check(ret >= 0);
        return fd_to_handle(ret);
    })

    IF_WINDOWS({
        HANDLE ret = 0;
        if (mode == FileMode_Read) ret = CreateFileA(path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (mode == FileMode_Write) ret = CreateFileA(path, GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (mode == FileMode_Create) ret = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (mode == FileMode_CreateExe) ret = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        check(ret);
        check(ret != INVALID_HANDLE_VALUE);
        return (File *)ret;
    })

    IF_WASM({ return 0; })
}

typedef enum {
    FileType_None,
    FileType_File,
    FileType_Directory,
    FileType_Other,
} FileType;

// Get file size
typedef struct {
    size_t size;
    time_t mtime;
    FileType type;
} FileInfo;

static FileInfo fs_stat(char *path) {
    FileInfo info = {};

    IF_LINUX({
        struct linux_stat sb = {};
        check(linux_lstat(path, &sb) == 0);
        info.size = sb.st_size;
        info.mtime = time_from_ns(sb.st_mtime, sb.st_mtime_nsec);
        info.type = FileType_Other;
        u32 file_type = sb.st_mode & S_IFMT;
        if (file_type == S_IFREG) info.type = FileType_File;
        if (file_type == S_IFDIR) info.type = FileType_Directory;
    })

    IF_WINDOWS({
        WIN32_FILE_ATTRIBUTE_DATA winfo;
        check(GetFileAttributesExA(path, GetFileExInfoStandard, &winfo));
        info.size = ((size_t)winfo.nFileSizeHigh << 32) | (size_t)winfo.nFileSizeLow;
        info.mtime = 0; // TODO
        info.type = FileType_File;
        if (winfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            info.type = FileType_Directory;
        }
    })
    return info;
}

// Get current working directory
static char *fs_cwd(Memory *mem) {
    char buf[PATH_MAX];
    i64 len = 0;

    IF_LINUX({
        // Returns length including null byte, negative for error
        len = linux_getcwd(buf, sizeof(buf)) - 1;
    })

    IF_WINDOWS({
        // Returns length excluding null byte, zero for error
        len = GetCurrentDirectory(sizeof(buf), buf);
    })

    // Ensure valid result
    check_or(len > 0) return "";

    // Ensure null termination
    buf[len] = 0;

    // Copy into memory region
    return mem_clone(mem, buf, len + 1);
}

// Create an empty directory
static void fs_mkdir(char *path) {
    IF_LINUX({ check(linux_mkdir(path, 0755) == 0); })
    IF_WINDOWS({ check(CreateDirectoryA(path, NULL)); })
}

// Remove an empty directory
static void fs_rmdir(char *path) {
    IF_LINUX({ check(linux_rmdir(path) == 0); })
    IF_WINDOWS({ check(RemoveDirectoryA(path)); })
}

// Remove a file
static void fs_remove(char *path) {
    IF_LINUX({ check(linux_unlink(path) == 0); })
    IF_WINDOWS({ check(DeleteFileA(path)); })
}

static char *fs_realpath(Memory *mem, char *path) {
    char *result = path;

    IF_LINUX({
        char buffer[PATH_MAX];
        char *ret = realpath(path, buffer);
        check(ret);
        if (ret) result = ret;
    })

    return mem_clone(mem, result, str_len(result));
}

// Copy file contents from 'src' to 'dst'
static void fs_copy(char *src_path, char *dst_path) {
    File *src = fs_open(src_path, FileMode_Read);
    File *dst = fs_open(dst_path, FileMode_CreateExe);
    Buffer buffer = buf_stack(4 * 1024);
    for (;;) {
        size_t used = io_read_partial(src, buffer);
        if (error || used == 0) break;
        io_write(dst, buf_take(buffer, used));
        if (error) break;
    }
    io_close(src);
    io_close(dst);
}

static void fs_write(char *path, Buffer data) {
    File *fd = fs_open(path, FileMode_Create);
    io_write(fd, data);
    io_close(fd);
}

// Read entire file into memory (zero terminated)
static Buffer fs_read(Memory *mem, char *path) {
    size_t size = fs_stat(path).size;
    File *fd = fs_open(path, FileMode_Read);
    Buffer data = mem_buffer_zero_terminated(mem, size);
    io_read(fd, data);
    io_close(fd);
    return data;
}

// Read file contents into a string
static char *fs_read_str(Memory *mem, char *path) {
    return (char *)fs_read(mem, path).data;
}

// List directory contents
typedef void fs_list_cb(void *user, char *name, FileType type);

// List directory contents
static void fs_list(char *path, fs_list_cb *callback, void *user) {
    IF_LINUX({
        i32 dir = sys_open(path, O_RDONLY | O_DIRECTORY, 0);
        check_or(dir >= 0) return;

        for (;;) {
            u8 buffer[4 * 1024];
            i64 len = linux_getdents64(dir, (void *)buffer, sizeof(buffer));
            check(len >= 0);
            if (error) break;

            // End of stream
            if (len == 0) break;

            // Read entries
            void *start = buffer;
            void *end = buffer + len;
            for (struct linux_dirent64 *ent = start; (void *)ent < end; ent = (void *)ent + ent->reclen) {
                // Skip '.' and '..'
                if (str_eq(ent->name, ".")) continue;
                if (str_eq(ent->name, "..")) continue;

                FileType type = FileType_Other;
                if (ent->type == DT_DIR) type = FileType_Directory;
                if (ent->type == DT_REG) type = FileType_File;

                callback(user, ent->name, type);
                if (error) break;
            }
        }
        sys_close(dir);
    })

    IF_WINDOWS({
        // Construct a query: path + '\*'
        size_t path_len = str_len(path);
        char search_query[path_len + 3];
        ptr_copy(search_query, path, path_len);
        ptr_copy(search_query + path_len, "\\*", 2);
        search_query[path_len + 3] = 0;

        WIN32_FIND_DATAA find;
        HANDLE handle = FindFirstFileA(search_query, &find);
        if (handle == INVALID_HANDLE_VALUE) return;

        do {
            char *name = find.cFileName;

            // Skip "." and ".."
            if (str_eq(name, ".")) continue;
            if (str_eq(name, "..")) continue;

            FileType type = FileType_File;
            if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) type = FileType_Directory;

            callback(user, name, type);
            if (error) break;
        } while (FindNextFileA(handle, &find));
        FindClose(handle);
    })
}
