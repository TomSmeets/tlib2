// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// dwarf.h - Dwarf file format parser
#pragma once
#include "dwarf_types.h"
#include "elf.h"
#include "fmt.h"
#include "parse.h"
#include "str.h"

typedef struct {
    Dwarf_Attribute_Type name;
    Dwarf_Form form;
    i64 implicit_const_value;
} Dwarf_Abbrev_Attr;

typedef struct {
    Dwarf_Tag tag;
    bool has_children;
    u32 attr_count;
    Dwarf_Abbrev_Attr *attr_list;
} Dwarf_Abbrev;

static void dwarf_load(Memory *mem, Elf *elf, File *file) {
    Elf_Section *sect_info = elf_find_section(elf, ".debug_info");
    Elf_Section *sect_abbrev = elf_find_section(elf, ".debug_abbrev");
    assert(sect_info);
    assert(sect_abbrev);

    fmt_su(fout, "Section Offset: ", sect_info->offset, "\n");
    fmt_su(fout, "Section Size: ", sect_info->size, "\n");
    os_seek(file, sect_info->offset);
    u64 info_size = sect_info->size;
    u8 *info_data = os_read_alloc(mem, file, info_size);
    Parse *parse = parse_new(mem, info_data, info_size);

    u32 unit_length = parse_u32(parse);
    u16 version = parse_u16(parse);
    u8 unit_type = parse_u8(parse);
    u8 addr_size = parse_u8(parse);
    u32 debug_abbrev_offset = parse_u32(parse);

    fmt_su(fout, "unit_length: ", unit_length, "\n");
    fmt_su(fout, "version: ", version, "\n");
    fmt_su(fout, "unit_type: ", unit_type, "\n");
    fmt_su(fout, "addr_size: ", addr_size, "\n");
    fmt_su(fout, "debug_abbrev_offset: ", debug_abbrev_offset, "\n");
    assert_msg(unit_length < 0xfffffff0, "64 bit dwarf not supported yet");
    assert_msg(unit_type == 1, "Only supporting DW_UT_compile unit");

    os_seek(file, sect_abbrev->offset);
    u64 abbrev_size = sect_abbrev->size;
    u8 *abbrev_data = os_read_alloc(mem, file, abbrev_size);

    parse = parse_new(mem, abbrev_data, abbrev_size);

    u32 abbrev_count = 0;
    Dwarf_Abbrev abbrev_list[1024 * 4];
    for (;;) {
        if (parse_eof(parse)) break;

        u64 abbrev_code = parse_uleb128(parse);
        fmt_sx(fout, "Abbrev: ", abbrev_code, "\n");

        // Code 0 is used for padding
        if (abbrev_code == 0) continue;
        assert(abbrev_code < array_count(abbrev_list));
        if (abbrev_code >= abbrev_count) abbrev_count = abbrev_code + 1;

        Dwarf_Abbrev *abbrev = &abbrev_list[abbrev_code];
        abbrev->tag = parse_uleb128(parse);
        fmt_sx(fout, "DIE: ", abbrev->tag, "\n");

        abbrev->has_children = parse_u8(parse);
        fmt_su(fout, "has_children: ", abbrev->has_children, "\n");

        u32 attr_count = 0;
        Dwarf_Abbrev_Attr attr_list[64];
        for (;;) {
            u64 attr_name = parse_uleb128(parse);
            u64 attr_form = parse_uleb128(parse);
            if (attr_name == 0 && attr_form == 0) break;
            Dwarf_Abbrev_Attr *attr = &attr_list[attr_count++];
            attr->name = attr_name;
            attr->form = attr_form;

            fmt_s(fout, "attr: ");
            fmt_s(fout, dwarf_attribute_type_to_string(attr_name));
            fmt_s(fout, " -> ");
            fmt_s(fout, dwarf_form_to_string(attr_form));
            fmt_s(fout, "\n");

            if (attr_form == DW_FORM_implicit_const) {
                attr->implicit_const_value = parse_ileb128(parse);
                fmt_si(fout, "Implicit const: ", attr->implicit_const_value, "\n");
            }
        }

        abbrev->attr_count = attr_count;
        abbrev->attr_list = mem_clone(mem, attr_list, attr_count * sizeof(attr_list[0]));
        fmt_su(fout, "Count: ", attr_count, "\n");
    }
    fmt_su(fout, "Total Count: ", abbrev_count, "\n");

    abbrev->attr_count = attr_count;
    abbrev->attr_list = mem_clone(mem, attr_list, attr_count * sizeof(attr_list[0]));
}
