#pragma once
#include "elf.h"

#define MIN(A, B) ((A) <= (B) ? (A) : (B))

typedef struct {
    Library *lib;
    Elf *elf;

    // Methods
    void (*app_main)(u32 argc, const char **argv);
    void (*app_update)(void);
} Hot;

static Hot *hot_new(void) {
    Hot *hot = os_alloc(sizeof(Hot));
    return hot;
}

// Load a (new) version of the library
static void hot_load(Hot *hot, const char *path) {
    Library *lib = os_dlopen(path);
    File *file = os_open(path);
    Elf *elf = elf_load(file);
    os_close(file);

    if(hot->lib) {
        void *base_old = elf_base(hot->elf, hot->lib);
        void *base_new = elf_base(elf, lib);

        const char *sections[] = {
            ".data",
            ".bss",
        };

        // Copy over global variable data form old to new
        // Assuming same layout (works most of the time)
        for(u32 i = 0; i < array_count(sections); ++i) {
            Elf_Section *sect_old  = elf_find_section(hot->elf, sections[i]);
            Elf_Section *sect_new  = elf_find_section(elf, sections[i]);
            u32 size = MIN(sect_old->size, sect_new->size);
            std_memcpy(base_old + sect_old->addr, base_new + sect_new->addr, size);
        }
    }

    hot->lib = lib;
    hot->elf = elf;
    hot->app_main = os_dlsym(lib, "app_main");
    hot->app_update = os_dlsym(lib, "app_update");
}
