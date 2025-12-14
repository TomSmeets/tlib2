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

typedef struct {
    u32 count;
    Dwarf_Abbrev *abbrev;
} Dwarf_Abbrev_List;

typedef struct {
    Memory *mem;
    File *file;
    Elf *elf;
    Buffer sect_info;
    Buffer sect_abbrev;
    Buffer sect_str;
    Buffer sect_str_offsets;
} Dwarf_File;

static Buffer elf_read_section(Memory *mem, char *name, Elf *elf, File *file) {
    Elf_Section *sect = elf_find_section(elf, name);
    if (!sect) return (Buffer){0, 0};
    os_seek(file, sect->offset);
    u64 size = sect->size;
    u8 *data = os_read_alloc(mem, file, size);
    return (Buffer){data, size};
}

static Dwarf_File *dwarf_open(Memory *mem, Elf *elf, File *file) {
    Dwarf_File *dwarf = mem_struct(mem, Dwarf_File);
    dwarf->mem = mem;
    dwarf->file = file;
    dwarf->elf = elf;
    dwarf->sect_info = elf_read_section(mem, ".debug_info", elf, file);
    dwarf->sect_abbrev = elf_read_section(mem, ".debug_abbrev", elf, file);
    dwarf->sect_str = elf_read_section(mem, ".debug_str", elf, file);
    dwarf->sect_str_offsets = elf_read_section(mem, ".debug_str_offsets", elf, file);
    return dwarf;
}

static Dwarf_Abbrev_List dwarf_load_abbrev(Dwarf_File *file) {
    // debug_abbrev contains the structure of each DIE
    Parse *parse = parse_new(file->mem, file->sect_abbrev.data, file->sect_abbrev.size);

    // Allocate a big enogh array for abbreviatoon list
    u32 abbrev_capacity = 1024 * 4;
    u32 abbrev_count = 0;
    Dwarf_Abbrev *abbrev_list = mem_array(file->mem, Dwarf_Abbrev, abbrev_capacity);
    for (;;) {
        if (parse_eof(parse)) break;
        u64 abbrev_code = parse_uleb128(parse);

        // Code 0 is used for padding
        if (abbrev_code == 0) continue;
        assert(abbrev_code < abbrev_capacity);
        if (abbrev_code >= abbrev_count) abbrev_count = abbrev_code + 1;

        Dwarf_Abbrev *abbrev = &abbrev_list[abbrev_code];
        abbrev->tag = parse_uleb128(parse);
        abbrev->has_children = parse_u8(parse);
        u32 attr_count = 0;
        Dwarf_Abbrev_Attr attr_list[64];
        for (;;) {
            u64 attr_name = parse_uleb128(parse);
            u64 attr_form = parse_uleb128(parse);
            if (attr_name == 0 && attr_form == 0) break;
            Dwarf_Abbrev_Attr *attr = &attr_list[attr_count++];
            attr->name = attr_name;
            attr->form = attr_form;
            if (attr_form == DW_FORM_implicit_const) {
                attr->implicit_const_value = parse_ileb128(parse);
            }
        }
        abbrev->attr_count = attr_count;
        abbrev->attr_list = mem_clone(file->mem, attr_list, attr_count * sizeof(Dwarf_Abbrev_Attr));
    }
    return (Dwarf_Abbrev_List){
        .count = abbrev_count,
        .abbrev = abbrev_list,
    };
}

static u64 parse_u64_form(Parse *parse, Dwarf_Form form) {
    switch (form) {
    case DW_FORM_strx:
    case DW_FORM_addrx:
    case DW_FORM_ref_udata:
    case DW_FORM_udata:
    case DW_FORM_rnglistx:
        return parse_uleb128(parse);
    case DW_FORM_sdata:
        return parse_ileb128(parse);
    case DW_FORM_strx1:
    case DW_FORM_data1:
    case DW_FORM_addrx1:
    case DW_FORM_ref1:
        return parse_u8(parse);
    case DW_FORM_strx2:
    case DW_FORM_data2:
    case DW_FORM_addrx2:
    case DW_FORM_ref2:
        return parse_u16(parse);
    case DW_FORM_addrx3:
        return parse_u24(parse);
    case DW_FORM_strx4:
    case DW_FORM_data4:
    case DW_FORM_addrx4:
    case DW_FORM_ref4:
        return parse_u32(parse);
    case DW_FORM_data8:
    case DW_FORM_ref8:
        return parse_u64(parse);
    case DW_FORM_data16:
        (void)parse_u64(parse);
        return parse_u64(parse);
    default:
        assert(0);
        return 0;
    }
}

typedef struct Dwarf_Die Dwarf_Die;
typedef struct Dwarf_Die_Attr Dwarf_Die_Attr;

struct Dwarf_Die {
    Dwarf_Die_Attr *attr_list;
    Dwarf_Die *next;
    Dwarf_Die *child;
};

struct Dwarf_Die_Attr {
    Dwarf_Attribute_Type name;
    union {
        u64 data;
        char *str;
        Dwarf_Die *ref;
    } value;
    Dwarf_Die_Attr *next;
};

static void dwarf_load_die(Dwarf_File *dwarf, Dwarf_Abbrev_List *abbrev) {
    Parse *parse = parse_new(dwarf->mem, dwarf->sect_info.data, dwarf->sect_info.size);

    u32 unit_length = parse_u32(parse);
    fmt_su(fout, "unit_length: ", unit_length, "\n");
    assert_msg(unit_length < 0xfffffff0, "64 bit dwarf not supported yet");

    u16 version = parse_u16(parse);
    fmt_su(fout, "version: ", version, "\n");
    assert_msg(version == 5, "Only supports DWARF5");

    u8 unit_type = parse_u8(parse);
    fmt_su(fout, "unit_type: ", unit_type, "\n");
    assert_msg(unit_type == DW_UT_compile, "Only supports DW_UT_compile unit");

    u8 addr_size = parse_u8(parse);
    fmt_su(fout, "addr_size: ", addr_size, "\n");
    assert(addr_size == 8);

    u32 debug_abbrev_offset = parse_u32(parse);
    fmt_su(fout, "debug_abbrev_offset: ", debug_abbrev_offset, "\n");
    assert(debug_abbrev_offset == 0);

    while (1) {
        if(parse_eof(parse)) break;

        u64 off = parse->cursor;
        u64 die_abbrev_id = parse_uleb128(parse);
        // 0 is reserved for alignment
        if (die_abbrev_id == 0) continue;

        fmt_s(fout, "\n");
        fmt_su(fout, "Abbrev: ", die_abbrev_id, "\n");
        fmt_su(fout, "Offset: ", off, "\n");

        Dwarf_Abbrev *die_abbrev = abbrev->abbrev + die_abbrev_id;
        fmt_ss(fout, "", dwarf_tag_to_string(die_abbrev->tag), "\n");
        for (u32 i = 0; i < die_abbrev->attr_count; ++i) {
            Dwarf_Abbrev_Attr *attr = die_abbrev->attr_list + i;
            fmt_s(fout, "  ");
            fmt_s(fout, dwarf_attribute_type_to_string(attr->name));
            fmt_s(fout, " ");
            fmt_pad_line(fout, 30, ' ');
            fmt_s(fout, dwarf_form_to_string(attr->form));
            fmt_s(fout, " ");
            fmt_pad_line(fout, 55, ' ');

            Dwarf_Die_Attr die_attr = {};
            die_attr.name = attr->name;

            switch (attr->form) {
            case DW_FORM_strx:
            case DW_FORM_strx1:
            case DW_FORM_strx2:
            case DW_FORM_strx4: {
                u64 off = parse_u64_form(parse, attr->form);
                fmt_su(fout, "offset = ", off, "");
                die_attr.value.str = "";
            } break;
            case DW_FORM_udata:
            case DW_FORM_sdata:
            case DW_FORM_data1:
            case DW_FORM_data2:
            case DW_FORM_data4:
            case DW_FORM_data8:
            case DW_FORM_data16:
                die_attr.value.data = parse_u64_form(parse, attr->form);
                fmt_su(fout, "data = ", die_attr.value.data, "");
                break;
            case DW_FORM_sec_offset:
                die_attr.value.data = parse_u32(parse); // ?
                break;
            case DW_FORM_addr:
                if (addr_size == 8) die_attr.value.data = parse_u64(parse);
                if (addr_size == 4) die_attr.value.data = parse_u32(parse);
                fmt_su(fout, "addr = ", die_attr.value.data, "");
                break;
            case DW_FORM_addrx:
            case DW_FORM_addrx1:
            case DW_FORM_addrx2:
            case DW_FORM_addrx3:
            case DW_FORM_addrx4: {
                u64 data = parse_u64_form(parse, attr->form);
                fmt_su(fout, "addr = ", data, "");
            } break;
            case DW_FORM_ref_udata:
            case DW_FORM_ref1:
            case DW_FORM_ref2:
            case DW_FORM_ref4:
            case DW_FORM_ref8: {
                u64 data = parse_u64_form(parse, attr->form);
                fmt_su(fout, "ref = ", data, "");
            } break;
            case DW_FORM_exprloc: {
                u64 len = parse_uleb128(parse);
                parse_data(parse, len);
                fmt_su(fout, "exprloc = ", len, "");
            } break;
            case DW_FORM_flag_present: {
                fmt_su(fout, "flag = ", 1, "");
            } break;
            case DW_FORM_flag: {
                u8 present = parse_u8(parse);
                fmt_su(fout, "flag = ", present, "");
            } break;
            case DW_FORM_rnglistx: {
                u64 ix = parse_uleb128(parse);
                fmt_su(fout, "range_ix = ", ix, "");
            } break;

            default: {
                fmt_s(fout, "?");
                assert(0);
            } break;
            }
            fmt_s(fout, "\n");
        }
    }
}

static void dwarf_load(Memory *mem, Elf *elf, File *file) {
    Dwarf_File *dwarf = dwarf_open(mem, elf, file);

    // Load abbreviation list
    Dwarf_Abbrev_List abbrev = dwarf_load_abbrev(dwarf);
    dwarf_load_die(dwarf, &abbrev);
}
