#include "core/os.h"
#include "core/str.h"
#include "core/type.h"
#include "elf/dwarf.h"
#include "elf/elf.h"
#include <dlfcn.h>
#include <memory.h>
#include <stdio.h>

void os_main(u32 argc, const char **argv) {
    if (argc != 2) os_fail("Usage: main <INPUT>");

    File *file = os_open(argv[1], Open_Read);
    assert(file);

    Elf *elf = elf_load(file);
    assert(elf);

    // List section names
    printf("Header:\n");
    printf("entry: %p\n", (void *)elf->entry);
    printf("Sections:\n");
    for (u32 i = 0; i < elf->section_count; i++) {
        printf("[%2d] %016llx %4llu %s\n", i, elf->sections[i].addr, elf->sections[i].size, elf->sections[i].name);
    }

    // (void)value_1;
    // printf("value_1: %p\n", &value_1);
    // printf("value_2: %p\n", &value_2);
    // printf("main:    %p\n", &main);
    // void *base = elf_base(elf, 0);
    // printf("base:    %p\n", base);
    // value_2 = 0x1234;

    // char *sections[] = {
    //     ".data",
    //     ".bss",
    // };

    // for (u32 i = 0; i < elf->section_count; i++) {
    //     Elf_Section *sect = elf->sections + i;
    //     bool ok = false;
    //     if (!(str_eq(sect->name, ".data") || str_eq(sect->name, ".bss"))) continue;

    //     printf("== SECTION %s ==\n", sect->name);
    //     for (u32 i = 0; i < sect->size; ++i) {
    //         u8 *p = base + sect->addr + i;
    //         printf("%16p %02x\n", p, *p);
    //     }
    // }

    // dwarf_load(elf, file);
    os_close(file);
    os_exit(0);
}
