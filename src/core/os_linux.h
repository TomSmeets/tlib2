// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#pragma once
#include "core/type.h"
#include "core/os_api.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

static void *linux_heap_start;

static void *os_alloc(u32 size) {
    // Reserve space if needed
    if(!linux_heap_start) {
        linux_heap_start = mmap(0, 1LLU << 40, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        assert(linux_heap_start != MAP_FAILED);
    }

    // Align size up to nearest 4k page
    u32 mask = 1024*4 - 1;
    size = (size + mask) & (~mask);

    void *ptr = linux_heap_start;
    linux_heap_start += size;
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

static i32 os_system(const char *command) {
    return system(command);
}

int main(i32 argc, const char **argv) {
    for (;;) os_main(argc, argv);
}
