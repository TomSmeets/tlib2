// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// hot.h - Hot-Reload compiled C programs
#pragma once
#include "elf.h"
#include "fmt.h"
#include "mem.h"

// Create a unique path to a temporary file
static char *os_mktmp(Memory *mem, char *prefix, char *suffix) {
    u64 time = os_time();
    Fmt *fmt = fmt_new(mem);
    fmt_s(fmt, prefix);
    fmt_u_ex(fmt, os_time(), 16, 0, 0);
    fmt_s(fmt, suffix);
    return fmt_end(fmt);
}

static void os_file_copy(char *src_path, char *dst_path) {
    File *src = os_open(src_path, Open_Read);
    File *dst = os_open(dst_path, Open_CreateExe);
    u8 buffer[4 * 1024];
    for (;;) {
        u64 bytes_read = 0;
        assert(os_read(src, buffer, sizeof(buffer), &bytes_read));
        if (bytes_read == 0) break;
        assert(os_write(dst, buffer, bytes_read, 0));
    }
    os_close(src);
    os_close(dst);
}

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
    char *unique_path = os_mktmp(hot->mem, "/tmp/hot_", ".so");
    os_file_copy(path, unique_path);

    // Load library
    Library *lib = os_dlopen(unique_path);
    assert(lib);
    assert(lib != hot->lib);

    // Read elf file info
    File *file = os_open(unique_path, Open_Read);
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
            std_memcpy(addr_new, addr_old, size_min);
        }
    }

    hot->lib = lib;
    hot->elf = elf;
    hot->symbol = symbol_new;
    return symbol_new;
}
