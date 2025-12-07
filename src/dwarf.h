// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// dwarf.h - Dwarf file format parser
#pragma once
#include "elf.h"
#include "str.h"
#include "parse.h"
#include "fmt.h"

typedef enum {
    DW_TAG_array_type = 0x01,
    DW_TAG_class_type = 0x02,
    DW_TAG_entry_point = 0x03,
    DW_TAG_enumeration_type = 0x04,
    DW_TAG_formal_parameter = 0x05,
    DW_TAG_imported_declaration = 0x08,
    DW_TAG_label = 0x0a,
    DW_TAG_lexical_block = 0x0b,
    DW_TAG_member = 0x0d,
    DW_TAG_pointer_type = 0x0f,
    DW_TAG_reference_type = 0x10,
    DW_TAG_compile_unit = 0x11,
    DW_TAG_string_type = 0x12,
    DW_TAG_structure_type = 0x13,
    DW_TAG_subroutine_type = 0x15,
    DW_TAG_typedef = 0x16,
    DW_TAG_union_type = 0x17,
    DW_TAG_unspecified_parameters = 0x18,
    DW_TAG_variant = 0x19,
    DW_TAG_common_block = 0x1a,
    DW_TAG_common_inclusion = 0x1b,
    DW_TAG_inheritance = 0x1c,
    DW_TAG_inlined_subroutine = 0x1d,
    DW_TAG_module = 0x1e,
    DW_TAG_ptr_to_member_type = 0x1f,
    DW_TAG_set_type = 0x20,
    DW_TAG_subrange_type = 0x21,
    DW_TAG_with_stmt = 0x22,
    DW_TAG_access_declaration = 0x23,
    DW_TAG_base_type = 0x24,
    DW_TAG_catch_block = 0x25,
    DW_TAG_const_type = 0x26,
    DW_TAG_constant = 0x27,
    DW_TAG_enumerator = 0x28,
    DW_TAG_file_type = 0x29,
    DW_TAG_friend = 0x2a,
    DW_TAG_namelist = 0x2b,
    DW_TAG_namelist_item = 0x2c,
    DW_TAG_packed_type = 0x2d,
    DW_TAG_subprogram = 0x2e,
    DW_TAG_template_type_parameter = 0x2f,
    DW_TAG_template_value_parameter = 0x30,
    DW_TAG_thrown_type = 0x31,
    DW_TAG_try_block = 0x32,
    DW_TAG_variant_part = 0x33,
    DW_TAG_variable = 0x34,
    DW_TAG_volatile_type = 0x35,
    DW_TAG_dwarf_procedure = 0x36,
    DW_TAG_restrict_type = 0x37,
    DW_TAG_interface_type = 0x38,
    DW_TAG_namespace = 0x39,
    DW_TAG_imported_module = 0x3a,
    DW_TAG_unspecified_type = 0x3b,
    DW_TAG_partial_unit = 0x3c,
    DW_TAG_imported_unit = 0x3d,
    DW_TAG_condition = 0x3f,
    DW_TAG_shared_type = 0x40,
    DW_TAG_type_unit = 0x41,
    DW_TAG_rvalue_reference_type = 0x42,
    DW_TAG_template_alias = 0x43,
    DW_TAG_coarray_type = 0x44,
    DW_TAG_generic_subrange = 0x45,
    DW_TAG_dynamic_type = 0x46,
    DW_TAG_atomic_type = 0x47,
    DW_TAG_call_site = 0x48,
    DW_TAG_call_site_parameter = 0x49,
    DW_TAG_skeleton_unit = 0x4a,
    DW_TAG_immutable_type = 0x4b,
    DW_TAG_lo_user = 0x4080,
    DW_TAG_hi_user = 0xffff,
} DIE_Tag;

typedef enum {
    DW_FORM_addr = 0x01,           // address
    DW_FORM_block2 = 0x03,         // block
    DW_FORM_block4 = 0x04,         // block
    DW_FORM_data2 = 0x05,          // constant
    DW_FORM_data4 = 0x06,          // constant
    DW_FORM_data8 = 0x07,          // constant
    DW_FORM_string = 0x08,         // string
    DW_FORM_block = 0x09,          // block
    DW_FORM_block1 = 0x0a,         // block
    DW_FORM_data1 = 0x0b,          // constant
    DW_FORM_flag = 0x0c,           // flag
    DW_FORM_sdata = 0x0d,          // constant
    DW_FORM_strp = 0x0e,           // string
    DW_FORM_udata = 0x0f,          // constant
    DW_FORM_ref_addr = 0x10,       // reference
    DW_FORM_ref1 = 0x11,           // reference
    DW_FORM_ref2 = 0x12,           // reference
    DW_FORM_ref4 = 0x13,           // reference
    DW_FORM_ref8 = 0x14,           // reference
    DW_FORM_ref_udata = 0x15,      // reference
    DW_FORM_indirect = 0x16,       // (see Section 7.5.3 on page 203)
    DW_FORM_sec_offset = 0x17,     // addrptr, lineptr, loclist, loclistsptr, macptr, rnglist, rnglistsptr, stroffsetsptr
    DW_FORM_exprloc = 0x18,        // exprloc
    DW_FORM_flag_present = 0x19,   // flag
    DW_FORM_strx = 0x1a,           // string
    DW_FORM_addrx = 0x1b,          // address
    DW_FORM_ref_sup4 = 0x1c,       // reference
    DW_FORM_strp_sup = 0x1d,       // string
    DW_FORM_data16 = 0x1e,         // constant
    DW_FORM_line_strp = 0x1f,      // string
    DW_FORM_ref_sig8 = 0x20,       // reference
    DW_FORM_implicit_const = 0x21, // constant
    DW_FORM_loclistx = 0x22,       // loclist
    DW_FORM_rnglistx = 0x23,       // rnglist
    DW_FORM_ref_sup8 = 0x24,       // reference
    DW_FORM_strx1 = 0x25,          // string
    DW_FORM_strx2 = 0x26,          // string
    DW_FORM_strx3 = 0x27,          // string
    DW_FORM_strx4 = 0x28,          // string
    DW_FORM_addrx1 = 0x29,         // address
    DW_FORM_addrx2 = 0x2a,         // address
    DW_FORM_addrx3 = 0x2b,         // address
    DW_FORM_addrx4 = 0x2c,         // address
} Dwarf_Form;

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

    for(;;) {
        if (parse_eof(parse)) break;
        u64 abbrev_code = parse_uleb128(parse);
        fmt_sx(fout, "Abbrev: ", abbrev_code, "\n");
        if (abbrev_code == 0) break;
        if (parse_eof(parse)) break;
        u64 die_tag = parse_uleb128(parse);
        fmt_sx(fout, "DIE: ", die_tag, "\n");
        u8 has_children = parse_u8(parse);
        fmt_su(fout, "has_children: ", has_children, "\n");

        for (;;) {
            u64 attr_name = parse_uleb128(parse);
            u64 attr_form = parse_uleb128(parse);
            fmt_s(fout, "Attr: ");
            fmt_x(fout, attr_name);
            fmt_s(fout, " ");
            fmt_x(fout, attr_form);
            fmt_s(fout, "\n");
            if (attr_form == DW_FORM_implicit_const) {
                i64 form_value = parse_ileb128(parse);
                fmt_si(fout, "Implicit const: ", form_value, "\n");
            }
            if (attr_name == 0 && attr_form == 0) break;
        }
    }
}
