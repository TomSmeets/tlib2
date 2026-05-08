// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// dl.h - Dynamic Linking
#pragma once
#include "os_headers.h"

// Dynamic library is just another opaque handle
typedef struct Library Library;

// Open a library by name or full path
static Library *dl_open(char *path) {
#if OS_LINUX
    return (Library *)dlopen(path, RTLD_LOCAL | RTLD_NOW);
#elif OS_WINDOWS
    return (Library *)LoadLibrary(path);
#endif
}

// Lookup a symbol in a library
static void *dl_sym(Library *lib, char *sym) {
#if OS_LINUX
    return dlsym((void *)lib, sym);
#elif OS_WINDOWS
    return GetProcAddress((void *)lib, sym);
#endif
}

// Get base address of a library based on a pointer inside that library
static void *dl_base(void *ptr) {
#if OS_LINUX
    Dl_info info = {};
    int ret = dladdr(ptr, &info);
    check(ret != 0);
    return info.fbase;
#else
    return 0;
#endif
}
