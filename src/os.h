#pragma once
#include "type.h"

#define assert(cond) if(!(cond)) os_fail(#cond)
#define assert_msg(cond, msg) if(!(cond)) os_fail(msg)

static void *os_alloc(u32 size);
static void os_fail(const char *message) __attribute__((__noreturn__));

typedef struct File File;
static File *os_open(const char *path);
static void os_close(File *file);
static u32 os_read(File *file, void *data, u32 size);

typedef struct Library Library;
static Library *os_dlopen(const char *path);
static void *os_dlsym(Library *lib, const char *sym);


// Linux
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

static void *os_alloc(u32 size) {
    return malloc(size);
}

static void os_fail(const char *message) {
    fprintf(stderr, "Failed: %s\n", message);
    exit(1);
}

static File *os_open(const char *path) {
    return (File *) fopen(path, "rb");
}

static void os_close(File *file) {
    fclose((FILE *)file);
}

static u32 os_read(File *file, void *data, u32 size) {
    return fread(data, 1, size, (FILE *)file);
}

static void os_seek(File *file, u32 pos) {
    fseek((FILE *)file, pos, SEEK_SET);
}

static Library *os_dlopen(const char *path) {
    return (Library *)dlopen(path, RTLD_LOCAL | RTLD_NOW);
}
static void *os_dlsym(Library *lib, const char *sym) {
    return dlsym((void*) lib, sym);
}
