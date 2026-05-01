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

static Buffer elf_read_section(Memory *mem, char *name, Elf *elf) {
    Elf_Section *sect = elf_find_section(elf, name);
    check(sect);
    if (error) return buf_null();

    os_seek(elf->file, sect->offset);
    u64 size = sect->size;
    u8 *data = os_read_alloc(mem, elf->file, size);
    return (Buffer){data, size};
}

static Dwarf_File *dwarf_open(Memory *mem, Elf *elf) {
    Dwarf_File *dwarf = mem_struct(mem, Dwarf_File);
    dwarf->mem = mem;
    dwarf->file = elf->file;
    dwarf->elf = elf;
    dwarf->sect_info = elf_read_section(mem, ".debug_info", elf);
    dwarf->sect_abbrev = elf_read_section(mem, ".debug_abbrev", elf);
    dwarf->sect_str = elf_read_section(mem, ".debug_str", elf);
    dwarf->sect_str_offsets = elf_read_section(mem, ".debug_str_offsets", elf);
    return dwarf;
}

static Dwarf_Abbrev_List dwarf_load_abbrev(Dwarf_File *file) {
    // debug_abbrev contains the structure of each DIE
    Parse *parse = parse_new(file->mem, file->sect_abbrev.data, file->sect_abbrev.size);

    // Allocate a big enough array for abbreviation list
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
    print("unit_length: ", unit_length);
    check_or(unit_length < 0xfffffff0) return;

    u16 version = parse_u16(parse);
    print("version: ", version);
    check_or(version == 5) return;

    u8 unit_type = parse_u8(parse);
    print("unit_type: ", unit_type);
    check_or(unit_type == DW_UT_compile) return;

    u8 addr_size = parse_u8(parse);
    print("addr_size: ", addr_size);
    check_or(addr_size == 8) return;

    u32 debug_abbrev_offset = parse_u32(parse);
    print("debug_abbrev_offset: ", debug_abbrev_offset);
    check_or(debug_abbrev_offset == 0) return;

    while (1) {
        if (parse_eof(parse)) break;

        u64 off = parse->cursor;
        u64 die_abbrev_id = parse_uleb128(parse);
        // 0 is reserved for alignment
        if (die_abbrev_id == 0) continue;

        print("");
        print("Abbrev: ", die_abbrev_id);
        print("Offset: ", off);

        Dwarf_Abbrev *die_abbrev = abbrev->abbrev + die_abbrev_id;
        print(dwarf_tag_to_string(die_abbrev->tag));
        for (u32 i = 0; i < die_abbrev->attr_count; ++i) {
            Dwarf_Abbrev_Attr *attr = die_abbrev->attr_list + i;
            print(F_NoEOL, "  ", dwarf_attribute_type_to_string(attr->name), " ");
            // fmt_pad_line(fout, 30, ' ');
            print(F_NoEOL, dwarf_form_to_string(attr->form), " ");
            // fmt_pad_line(fout, 55, ' ');

            Dwarf_Die_Attr die_attr = {};
            die_attr.name = attr->name;

            switch (attr->form) {
            case DW_FORM_strx:
            case DW_FORM_strx1:
            case DW_FORM_strx2:
            case DW_FORM_strx4: {
                u64 off = parse_u64_form(parse, attr->form);
                print(F_NoEOL, "offset = ", off);
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
                print(F_NoEOL, "data = ", die_attr.value.data);
                break;
            case DW_FORM_sec_offset:
                die_attr.value.data = parse_u32(parse); // ?
                break;
            case DW_FORM_addr:
                if (addr_size == 8) die_attr.value.data = parse_u64(parse);
                if (addr_size == 4) die_attr.value.data = parse_u32(parse);
                print(F_NoEOL, "addr = ", die_attr.value.data);
                break;
            case DW_FORM_addrx:
            case DW_FORM_addrx1:
            case DW_FORM_addrx2:
            case DW_FORM_addrx3:
            case DW_FORM_addrx4: {
                u64 data = parse_u64_form(parse, attr->form);
                print(F_NoEOL, "addr = ", data);
            } break;
            case DW_FORM_ref_udata:
            case DW_FORM_ref1:
            case DW_FORM_ref2:
            case DW_FORM_ref4:
            case DW_FORM_ref8: {
                u64 data = parse_u64_form(parse, attr->form);
                print(F_NoEOL, "ref = ", data);
            } break;
            case DW_FORM_exprloc: {
                u64 len = parse_uleb128(parse);
                parse_data(parse, len);
                print(F_NoEOL, "exprloc = ", len);
            } break;
            case DW_FORM_flag_present: {
                print(F_NoEOL, "flag = ", 1);
            } break;
            case DW_FORM_flag: {
                u8 present = parse_u8(parse);
                print(F_NoEOL, "flag = ", present);
            } break;
            case DW_FORM_rnglistx: {
                u64 ix = parse_uleb128(parse);
                print(F_NoEOL, "range_ix = ", ix);
            } break;

            default: {
                print(F_NoEOL, "?");
                assert(0);
            } break;
            }
            print("");
        }
    }
}

static void dwarf_load(Memory *mem, Elf *elf) {
    Dwarf_File *dwarf = dwarf_open(mem, elf);

    // Load abbreviation list
    Dwarf_Abbrev_List abbrev = dwarf_load_abbrev(dwarf);
    dwarf_load_die(dwarf, &abbrev);
}
