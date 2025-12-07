// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// hot.h - Hot reloading helpers
#pragma once
#include "elf.h"
#include "fmt.h"
#include "mem.h"

typedef struct Hot Hot;
static Hot *hot_new(Memory *mem);

// Create a new unque temporary file path
static char *hot_mktmp(Hot *hot, char *prefix);

// Load a new application from a given path
static void hot_load(Hot *hot, char *path);

// Run `os_main` of the loaded applcation
static void hot_call(Hot *hot, u32 argc, char **argv);

// =======================================================
struct Hot {
    Memory *mem;
    Library *lib;
    Elf *elf;
    void (*os_main)(u32 argc, char **argv);
};

static Hot *hot_new(Memory *mem) {
    Hot *hot = mem_struct(mem, Hot);
    hot->mem = mem;
    return hot;
}

static char *hot_mktmp(Hot *hot, char *prefix) {
    u64 time = os_time();
    Fmt *fmt = fmt_new(hot->mem);
    fmt_s(fmt, prefix);
    fmt_s(fmt, "_");
    fmt_u_ex(fmt, os_time(), 16, 0, 0);
    fmt_s(fmt, ".so");
    return fmt_end(fmt);
}

// Load a (new) version of the library
static void hot_load(Hot *hot, char *path) {
    Library *lib = os_dlopen(path);
    assert(lib);

    File *file = os_open(path, Open_Read);
    Elf *elf = elf_load(hot->mem, file);
    os_close(file);

    // If application was already loaded
    if (hot->lib) {
        void *base_old = os_dlbase(hot->lib);
        void *base_new = os_dlbase(lib);

        char *sections[] = {
            ".data",
            ".bss",
        };

        // Copy over global variable data form old to new
        // Assuming same layout (works most of the time)
        for (u32 i = 0; i < array_count(sections); ++i) {
            Elf_Section *sect_old = elf_find_section(hot->elf, sections[i]);
            Elf_Section *sect_new = elf_find_section(elf, sections[i]);

            void *addr_old = base_old + sect_old->addr;
            void *addr_new = base_new + sect_new->addr;
            u32 size_old = sect_old->size;
            u32 size_new = sect_new->size;
            u32 size_min = MIN(size_old, size_new);
            std_memcpy(addr_new, addr_old, size_min);
        }
    }

    hot->lib = lib;
    hot->elf = elf;
    hot->os_main = os_dlsym(lib, "os_main");
}

static void hot_call(Hot *hot, u32 argc, char **argv) {
    if (!hot->os_main) return;
    hot->os_main(argc, argv);
}
