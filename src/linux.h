// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// linux.h - Platform syscall headers for Linux
#pragma once
#include "type.h"

// Basic Assumptions
static_assert(sizeof(long) == sizeof(i64));
static_assert(sizeof(int) == sizeof(i32));
static_assert(sizeof(void *) == sizeof(u64));

// Linux file descriptor to generic "handle" conversion
static void *linux_file(i32 fd) {
    if (fd < 0) return 0;
    return (void *)((intptr_t)fd + 1);
}

static i32 linux_fd(void *file) {
    if (!file) return -1;
    return (i32)((intptr_t)file - 1);
}

// Timespec conversion helpers
struct linux_timespec {
    i64 sec;
    i64 nsec;
};

struct linux_timeval {
    i64 sec;
    i64 usec;
};

// We use micro seconds, which should be enogh
static u64 time_from_ns(u64 sec, u64 nsec) {
    return sec * 1000 * 1000 + nsec / 1000;
}

static u64 time_from_timespec(struct linux_timespec *t) {
    return time_from_ns(t->sec, t->nsec);
}

static struct linux_timespec time_to_timespec(u64 time) {
    u64 sec = time / (1000 * 1000);
    u64 nsec = (time - sec * 1000 * 1000) * 1000;

    struct linux_timespec ts;
    ts.sec = sec;
    ts.nsec = nsec;
    return ts;
}

// Functions from libc
extern int system(const char *command);

#define RTLD_NOW 0x00002
#define RTLD_LOCAL 0
extern void *dlopen(const char *file, int mode);
extern void *dlsym(void *restrict handle, const char *restrict name);
extern char *dlerror(void);

typedef struct {
    const char *fname;
    void *fbase;
    const char *sname;
    void *saddr;
} Dl_info;
extern int dladdr(const void *__address, Dl_info *__info);

// =================== Syscalls ==============
static i64 linux_syscall6(i64 a0, i64 a1, i64 a2, i64 a3, i64 a4, i64 a5, i64 a6) {
    i64 ret;
    // Registers
    register i64 r0 __asm__("rax") = a0;
    register i64 r1 __asm__("rdi") = a1;
    register i64 r2 __asm__("rsi") = a2;
    register i64 r3 __asm__("rdx") = a3;
    register i64 r4 __asm__("r10") = a4;
    register i64 r5 __asm__("r8") = a5;
    register i64 r6 __asm__("r9") = a6;

    // https://stackoverflow.com/a/54957101
    // https://gitlab.com/x86-psABIs/x86-64-ABI
    __asm__ __volatile__(
        // Instruction
        "syscall"
        // Output registers a -> rax, '=' indicates write only
        // https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#Output-Operands
        // https://gcc.gnu.org/onlinedocs/gcc/Machine-Constraints.html
        : "=a"(ret)
        : "r"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5), "r"(r6)
        // Clobbers, these are not preserved
        : "rcx", "r11", "memory"
    );
    return ret;
}

// Same but fewer arguments, see linux_syscall6
static i64 linux_syscall5(i64 a0, i64 a1, i64 a2, i64 a3, i64 a4, i64 a5) {
    i64 ret;
    register i64 r0 __asm__("rax") = a0;
    register i64 r1 __asm__("rdi") = a1;
    register i64 r2 __asm__("rsi") = a2;
    register i64 r3 __asm__("rdx") = a3;
    register i64 r4 __asm__("r10") = a4;
    register i64 r5 __asm__("r8") = a5;
    __asm__ __volatile__("syscall" : "=a"(ret) : "r"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5) : "rcx", "r11", "memory");
    return ret;
}

// Same but fewer arguments, see linux_syscall6
static i64 linux_syscall4(i64 a0, i64 a1, i64 a2, i64 a3, i64 a4) {
    i64 ret;
    register i64 r0 __asm__("rax") = a0;
    register i64 r1 __asm__("rdi") = a1;
    register i64 r2 __asm__("rsi") = a2;
    register i64 r3 __asm__("rdx") = a3;
    register i64 r4 __asm__("r10") = a4;
    __asm__ __volatile__("syscall" : "=a"(ret) : "r"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r4) : "rcx", "r11", "memory");
    return ret;
}

// Same but fewer arguments, see linux_syscall6
static i64 linux_syscall3(i64 a0, i64 a1, i64 a2, i64 a3) {
    i64 ret;
    register i64 r0 __asm__("rax") = a0;
    register i64 r1 __asm__("rdi") = a1;
    register i64 r2 __asm__("rsi") = a2;
    register i64 r3 __asm__("rdx") = a3;
    __asm__ __volatile__("syscall" : "=a"(ret) : "r"(r0), "r"(r1), "r"(r2), "r"(r3) : "rcx", "r11", "memory");
    return ret;
}

// Same but fewer arguments, see linux_syscall6
static i64 linux_syscall2(i64 a0, i64 a1, i64 a2) {
    i64 ret;
    register i64 r0 __asm__("rax") = a0;
    register i64 r1 __asm__("rdi") = a1;
    register i64 r2 __asm__("rsi") = a2;
    __asm__ __volatile__("syscall" : "=a"(ret) : "r"(r0), "r"(r1), "r"(r2) : "rcx", "r11", "memory");
    return ret;
}

// Same but fewer arguments, see linux_syscall6
static i64 linux_syscall1(i64 a0, i64 a1) {
    i64 ret;
    register i64 r0 __asm__("rax") = a0;
    register i64 r1 __asm__("rdi") = a1;
    __asm__ __volatile__("syscall" : "=a"(ret) : "r"(r0), "r"(r1) : "rcx", "r11", "memory");
    return ret;
}

// Same but fewer arguments, see linux_syscall6
static i64 linux_syscall0(i64 a0) {
    i64 ret;
    register i64 r0 __asm__("rax") = a0;
    __asm__ __volatile__("syscall" : "=a"(ret) : "r"(r0) : "rcx", "r11", "memory");
    return ret;
}

// Linux Errors (if return value < 0, errno = -return)
#define EAGAIN 11

// ==== File IO Syscalls ====
#define O_RDONLY 00
#define O_WRONLY 01
#define O_RDWR 02

#define O_CREAT 0100
#define O_TRUNC 01000
#define O_APPEND 02000
#define O_NONBLOCK 04000
#define O_DIRECTORY 0200000
#define O_CLOEXEC 02000000

static i64 linux_read(i32 fd, void *buf, u64 size) {
    return linux_syscall3(0x00, fd, (i64)buf, size);
}

static i64 linux_write(i32 fd, const void *buf, u64 size) {
    return linux_syscall3(0x01, fd, (i64)buf, size);
}

static i32 linux_open(const char *path, i32 flags, u32 mode) {
    return linux_syscall3(0x02, (i64)path, flags, mode);
}

static i32 linux_close(i32 fd) {
    return linux_syscall1(0x03, fd);
}

// ==== Stat ====
struct linux_stat {
    u64 st_dev;
    u64 st_ino;
    u64 st_nlink;

    u32 st_mode;
    u32 st_uid;
    u32 st_gid;
    u32 __pad0;
    u64 st_rdev;
    i64 st_size;
    i64 st_blksize;
    i64 st_blocks;

    u64 st_atime;
    u64 st_atime_nsec;
    u64 st_mtime;
    u64 st_mtime_nsec;
    u64 st_ctime;
    u64 st_ctime_nsec;
    i64 __unused[3];
};

#define S_IFMT 0170000   // bit mask for the file type bit field
#define S_IFSOCK 0140000 // socket
#define S_IFLNK 0120000  // symbolic link
#define S_IFREG 0100000  // regular file
#define S_IFBLK 0060000  // block device
#define S_IFDIR 0040000  // directory
#define S_IFCHR 0020000  // character device
#define S_IFIFO 0010000  // FIFO

static i32 linux_fstat(i32 fd, struct linux_stat *buf) {
    return linux_syscall2(0x05, fd, (i64)buf);
}

static i32 linux_lstat(char *path, struct linux_stat *buf) {
    return linux_syscall2(0x06, (i64)path, (i64)buf);
}

// ==== Seek ====
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
static i64 linux_seek(i32 fd, i64 offset, u32 whence) {
    return linux_syscall3(0x08, fd, offset, whence);
}

// ==== Memory alloction (mmap) ====
#define PROT_READ 0x1
#define PROT_WRITE 0x2

#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED ((void *)-1)

static void *linux_mmap(void *addr, u64 len, i32 prot, i32 flags, i32 fd, i64 offset) {
    return (void *)linux_syscall6(0x09, (i64)addr, len, prot, flags, fd, offset);
}

// ==== Sleep ====
static i32 linux_nanosleep(const struct linux_timespec *duration, struct linux_timespec *remaining) {
    return linux_syscall2(0x23, (i64)duration, (i64)remaining);
}

// ==== Get current working directory ====
static i64 linux_getcwd(char *buf, u64 size) {
    return linux_syscall2(0x4f, (i64)buf, size);
}

// ==== Create and remove directory (mkdir) ====
static int linux_mkdir(const char *path, int mode) {
    return linux_syscall2(0x53, (i64)path, mode);
}

static int linux_rmdir(const char *path) {
    return linux_syscall1(0x54, (i64)path);
}

// ==== Remove file (unlink) ====
static int linux_unlink(const char *path) {
    return linux_syscall1(0x57, (i64)path);
}

// ==== Get Time ====
#define CLOCK_MONOTONIC 1
static i32 linux_clock_gettime(i32 clock_id, struct linux_timespec *time) {
    return linux_syscall2(0xe4, clock_id, (i64)time);
}

// vsyscalls version for getting time
// vsyhscalls are simpler than loading the vdso
// TODO: The libc version works in userspace using VDSO, maby cool to implement too
// extern i32 clock_gettime(i32 clock_id, struct linux_timespec *tp);
static i64 linux_gettimeofday(struct linux_timeval *time, void *tz) {
    return ((i64 (*)(struct linux_timeval *, void *))0xffffffffff600000)(time, tz);
}

// ==== Get directory entries (getdents) ====
#define DT_DIR 4
#define DT_REG 8

struct linux_dirent64 {
    u64 ino;
    i64 off;
    unsigned short reclen;
    unsigned char type;
    char name[];
};

static i64 linux_getdents64(i32 fd, struct linux_dirent64 *dirent, u32 count) {
    return linux_syscall3(0xd9, fd, (i64)dirent, count);
}

// ==== Exit ====
__attribute__((__noreturn__)) static void linux_exit_group(i32 error_code) {
    // Add infinite loop to make clang happy
    // (function should not return)
    for (;;) linux_syscall1(0xe7, error_code);
}

// ==== Get Random ====
static i64 linux_getrandom(void *buf, u64 size, u32 flags) {
    return linux_syscall3(0x13e, (i64)buf, size, flags);
}

// ==== INotify ====
#define IN_MODIFY 0x00000002
#define IN_CREATE 0x00000100
#define IN_DELETE 0x00000200
#define NAME_MAX 255

struct inotify_event {
    i32 wd;
    u32 mask;
    u32 cookie;
    u32 len;
    char name[];
};

static i32 linux_inotify_init(i32 flags) {
    return linux_syscall1(0x126, flags);
}

static i32 linux_inotify_add_watch(i32 fd, const char *path, u32 mask) {
    return linux_syscall3(0xfe, fd, (i64)path, mask);
}
