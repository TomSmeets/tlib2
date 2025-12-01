// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#pragma once
#include "core/os_api.h"
#include "core/type.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

static void *reserve_start;

static void *os_alloc(u32 size) {

    // Align size up to nearest 4k page
    u32 mask = 1024 * 4 - 1;
    size = (size + mask) & (~mask);

    // Reserve space if needed
    if (!reserve_start) {
        reserve_start = mmap(0, 1LLU << 40, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        assert(reserve_start != MAP_FAILED);
    }

    void *alloc = reserve_start;
    mprotect(alloc, size, PROT_READ | PROT_WRITE);
    reserve_start += size;
    return alloc;
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

static i32 os_system(const char *command) {
    return system(command);
}

int main(i32 argc, const char **argv) {
    for (;;) os_main(argc, argv);
}
