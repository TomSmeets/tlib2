#pragma once
#include "elf.h"
#include "str.h"

typedef struct {
    u32 unit_length1;
    // u32 unit_length1;
    u16 version;
    u32 abbrev_offset;
    u8 addr_size;
} __attribute__((packed)) Dwarf_CU;

static void dwarf_load(Elf *elf, File *file) {
    Elf_Section *sect_info   = elf_find_section(elf, ".debug_info");
    Elf_Section *sect_abbrev = elf_find_section(elf, ".debug_abbrev");
    assert(sect_info);
    assert(sect_abbrev);

    os_seek(file, sect_info->offset);
    Dwarf_CU *cu = os_read_alloc(file, sizeof(Dwarf_CU));
    printf("off: %016llx\n", sect_info->offset);
    printf("CU.len1: %016x\n", cu->unit_length1);
    // printf("CU.len2: %llu\n", cu->unit_length2);
    printf("version: %u\n", cu->version);
    (void)cu;
}
