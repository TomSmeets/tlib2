// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#pragma once
#include "core/os_api.h"
#include "core/type.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

// Main function
int main(i32 argc, const char **argv) {
    for (;;) os_main(argc, argv);
}

static void *os_alloc(u32 size) {
    void *ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(ptr != MAP_FAILED);
    return ptr;
}

static void os_fail(const char *message) {
    fprintf(stderr, "Failed: %s\n", message);
    exit(1);
}

static void os_exit(u32 code) {
    exit(code);
}


static File *os_open(const char *path) {
    return (File *)fopen(path, "rb");
}

static void os_close(File *file) {
    fclose((FILE *)file);
}

static u32 os_read(File *file, void *data, u32 size) {
    return fread(data, 1, size, (FILE *)file);
}

static u32 os_write(File *file, const void *data, u32 size) {
    return fwrite(data, 1, size, (FILE *)file);
}

static void os_seek(File *file, u32 pos) {
    fseek((FILE *)file, pos, SEEK_SET);
}


static Library *os_dlopen(const char *path) {
    return (Library *)dlopen(path, RTLD_LOCAL | RTLD_NOW);
}

static void *os_dlsym(Library *lib, const char *sym) {
    return dlsym((void *)lib, sym);
}

typedef struct {
    const char *fname;
    void *fbase;
    const char *sname;
    void *saddr;
} Dl_info;

extern int dladdr(const void *__address, Dl_info *__info);

static void *os_dlbase(Library *lib) {
    void *addr = os_dlsym(lib, "os_main");
    Dl_info info;
    dladdr(addr, &info);
    return info.fbase;
}

static i32 os_system(const char *command) {
    return system(command);
}
