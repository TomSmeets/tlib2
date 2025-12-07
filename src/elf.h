// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// elf.h - Elf File parsing
#pragma once
#include "mem.h"
#include "os.h"
#include "str.h"
#include "type.h"

// API
typedef struct {
    char *name;
    u64 offset; // File address
    u64 addr;   // Virtual address
    u64 size;   // Size of section
} Elf_Section;

typedef struct {
    u64 entry;
    u32 section_count;
    Elf_Section *sections;
} Elf;

// Find section with name
// or return null when not present
static Elf_Section *elf_find_section(Elf *elf, char *name) {
    for (u32 i = 0; i < elf->section_count; i++) {
        Elf_Section *sect = elf->sections + i;
        if (str_eq(sect->name, name)) return sect;
    }
    return 0;
}

// Implementation
typedef struct {
    u8 ident[16];  // Magic number and other info
    u16 type;      // Object file type
    u16 machine;   // Architecture
    u32 version;   // Object file version
    u64 entry;     // Entry point virtual address
    u64 phoff;     // Program header table file offset
    u64 shoff;     // Section header table file offset
    u32 flags;     // Processor-specific flags
    u16 ehsize;    // ELF header size in bytes
    u16 phentsize; // Program header table entry size
    u16 phnum;     // Program header table entry count
    u16 shentsize; // Section header table entry size
    u16 shnum;     // Section header table entry count
    u16 shstrndx;  // Section header string table index
} Elf64_Ehdr;

typedef struct {
    u32 name;      // Section name (index into string table)
    u32 type;      // Section type
    u64 flags;     // Section flags
    u64 addr;      // Virtual address in memory
    u64 offset;    // Offset in file
    u64 size;      // Size of section
    u32 link;      // Link to another section
    u32 info;      // Additional info
    u64 addralign; // Alignment
    u64 entsize;   // Entry size if section holds table
} Elf64_Shdr;

static void *os_read_alloc(Memory *mem, File *file, u32 size) {
    u8 *data = mem_array(mem, u8, size);
    if (os_read(file, data, size) != size) {
        return 0;
    }
    return data;
}

static Elf *elf_load(Memory *mem, File *file) {
    Elf64_Ehdr header;
    assert(os_read(file, &header, sizeof(header)) == sizeof(header));

    // Check ELF magic number
    assert(header.ident[0] == 0x7f);
    assert(header.ident[1] == 'E');
    assert(header.ident[2] == 'L');
    assert(header.ident[3] == 'F');

    Elf *elf = mem_struct(mem, Elf);
    elf->entry = header.entry;
    elf->section_count = header.shnum;
    elf->sections = mem_array(mem, Elf_Section, elf->section_count);

    // Read section headers
    os_seek(file, header.shoff);
    Elf64_Shdr *table = os_read_alloc(mem, file, header.shnum * sizeof(Elf64_Shdr));
    assert(table);

    // Read section header string table
    Elf64_Shdr *strtab = &table[header.shstrndx];
    os_seek(file, strtab->offset);
    char *str = os_read_alloc(mem, file, strtab->size);
    assert(str);

    for (u32 i = 0; i < elf->section_count; i++) {
        elf->sections[i].name = str + table[i].name;
        elf->sections[i].offset = table[i].offset;
        elf->sections[i].addr = table[i].addr;
        elf->sections[i].size = table[i].size;
    }

    return elf;
}
