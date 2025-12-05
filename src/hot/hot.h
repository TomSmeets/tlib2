// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#pragma once
#include "core/mem.h"
#include "elf/elf.h"
#include "core/fmt.h"

typedef struct Hot Hot;
static Hot *hot_new(Memory *mem);
static void hot_load(Hot *hot, const char *path);
static void hot_call(Hot *hot, u32 argc, const char **argv);

// =======================================================
struct Hot {
    Library *lib;
    Elf *elf;
    void (*os_main)(u32 argc, const char **argv);
};

static Hot *hot_new(Memory *mem) {
    return mem_struct(mem, Hot);
}

// Load a (new) version of the library
static void hot_load(Hot *hot, const char *path) {
    fmt_s(stdout, "Loading: ");
    fmt_s(stdout, path);
    fmt_s(stdout, "\n");
    Library *lib = os_dlopen(path);
    assert(lib);
    File *file = os_open(path, Open_Read);
    Elf *elf = elf_load(file);
    os_close(file);

    // If application was already loaded
    if (hot->lib) {
        void *base_old = os_dlbase(hot->lib);
        void *base_new = os_dlbase(lib);

        const char *sections[] = {
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
            fmt_ss(stdout, "Section ", sections[i], "\n");

            // fmt_s(stdout, "  Base old:");
            // fmt_sp(stdout, "addr=", addr_old, " ");
            // fmt_su(stdout, "size=", size_old, "\n");
            std_memcpy(addr_new, addr_old, size_min);
        }
    }

    hot->lib = lib;
    hot->elf = elf;
    hot->os_main = os_dlsym(lib, "os_main");
}

static void hot_call(Hot *hot, u32 argc, const char **argv) {
    if (!hot->os_main) return;
    hot->os_main(argc, argv);
}
