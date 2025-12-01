// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#pragma once
#include "elf/elf.h"

typedef struct {
    Library *lib;
    Elf *elf;
    void (*os_main)(u32 argc, const char **argv);
} Hot;

static Hot *hot_new(void) {
    Hot *hot = os_alloc(sizeof(Hot));
    hot->lib = 0;
    hot->elf = 0;
    hot->os_main = 0;
    return hot;
}

static void *os_dlbase(Library *lib) {
    void *addr = os_dlsym(lib, "os_main");
    Dl_info info;
    dladdr(addr,&info);
    return info.dli_fbase;
}

// Load a (new) version of the library
static void hot_load(Hot *hot, const char *path) {
    printf("loading: %s\n", path);
    Library *lib = os_dlopen(path);
    assert(lib);
    File *file = os_open(path);
    Elf *elf = elf_load(file);
    os_close(file);

    // If application was already loaded
    if(hot->lib) {
        void *base_old = os_dlbase(hot->lib);
        void *base_new = os_dlbase(lib);

        const char *sections[] = {
            ".data",
            ".bss",
        };

        // Copy over global variable data form old to new
        // Assuming same layout (works most of the time)
        for(u32 i = 0; i < array_count(sections); ++i) {
            Elf_Section *sect_old  = elf_find_section(hot->elf, sections[i]);
            Elf_Section *sect_new  = elf_find_section(elf, sections[i]);

            void *addr_old = base_old + sect_old->addr;
            void *addr_new = base_new + sect_new->addr;
            u32 size_old = sect_old->size;
            u32 size_new = sect_new->size;
            u32 size_min = MIN(size_old, size_new);
            printf("Section %s\n",  sections[i]);
            printf("  Base old: addr=%p size=%d\n", addr_old, size_old);
            printf("  Base new: addr=%p size=%d\n", addr_new, size_new);
            std_memcpy(addr_new, addr_old, size_min);
        }
    }

    hot->lib = lib;
    hot->elf = elf;
    hot->os_main = os_dlsym(lib, "os_main");
}
