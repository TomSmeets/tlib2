// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// hot.h - Hot-Reload compiled C programs
#pragma once
#include "elf.h"
#include "fmt.h"
#include "mem.h"
#include "os2.h"
#include "time.h"

typedef struct {
    Memory *mem;
    Library *lib;
    Elf *elf;
    void *symbol;
} Hot;

static Hot *hot_new(Memory *mem) {
    Hot *hot = mem_struct(mem, Hot);
    hot->mem = mem;
    return hot;
}

// Load a (new) version of the library
static void *hot_load(Hot *hot, char *path, char *symbol) {
    // Path must be copied to a unique location
    time_t time = time_now();
    char *unique_path = fstr(hot->mem, "/tmp/hot_", O(.base = 16), (u64)time, ".so");
    os_file_copy(path, unique_path);

    // Load library
    Library *lib = dl_open(unique_path);
    assert(lib);
    assert(lib != hot->lib);

    // Read elf file info
    File *file = os_open(unique_path, FileMode_Read);
    Elf *elf = elf_load(hot->mem, file);
    os_close(file);

    void *symbol_new = os_dlsym(lib, symbol);

    // If application was already loaded
    if (hot->lib) {
        void *base_old = os_dlbase(hot->symbol);
        void *base_new = os_dlbase(symbol_new);

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
            ptr_copy(addr_new, addr_old, size_min);
        }
    }

    hot->lib = lib;
    hot->elf = elf;
    hot->symbol = symbol_new;
    return symbol_new;
}
