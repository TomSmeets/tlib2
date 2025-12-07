// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// dwarf.h - Dwarf file format parser
#pragma once
#include "elf.h"
#include "fmt.h"
#include "parse.h"
#include "str.h"

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

typedef enum {
    DW_AT_sibling = 0x01,                 // reference
    DW_AT_location = 0x02,                // exprloc, loclist
    DW_AT_name = 0x03,                    // string
    DW_AT_ordering = 0x09,                // constant
    DW_AT_byte_size = 0x0b,               // constant, exprloc, reference
    DW_AT_bit_size = 0x0d,                // constant, exprloc, reference
    DW_AT_stmt_list = 0x10,               // lineptr
    DW_AT_low_pc = 0x11,                  // address
    DW_AT_high_pc = 0x12,                 // address, constant
    DW_AT_language = 0x13,                // constant
    DW_AT_discr = 0x15,                   // reference
    DW_AT_discr_value = 0x16,             // constant
    DW_AT_visibility = 0x17,              // constant
    DW_AT_import = 0x18,                  // reference
    DW_AT_string_length = 0x19,           // exprloc, loclist, reference
    DW_AT_common_reference = 0x1a,        // reference
    DW_AT_comp_dir = 0x1b,                // string
    DW_AT_const_value = 0x1c,             // block, constant, string
    DW_AT_containing_type = 0x1d,         // reference
    DW_AT_default_value = 0x1e,           // constant, reference, flag
    DW_AT_inline = 0x20,                  // constant
    DW_AT_is_optional = 0x21,             // flag
    DW_AT_lower_bound = 0x22,             // constant, exprloc, reference
    DW_AT_producer = 0x25,                // string
    DW_AT_prototyped = 0x27,              // flag
    DW_AT_return_addr = 0x2a,             // exprloc, loclist
    DW_AT_start_scope = 0x2c,             // constant, rnglist
    DW_AT_bit_stride = 0x2e,              // constant, exprloc, reference
    DW_AT_upper_bound = 0x2f,             // constant, exprloc, reference
    DW_AT_abstract_origin = 0x31,         // reference
    DW_AT_accessibility = 0x32,           // constant
    DW_AT_address_class = 0x33,           // constant
    DW_AT_artificial = 0x34,              // flag
    DW_AT_base_types = 0x35,              // reference
    DW_AT_calling_convention = 0x36,      // constant
    DW_AT_count = 0x37,                   // constant, exprloc, reference
    DW_AT_data_member_location = 0x38,    // constant, exprloc, loclist
    DW_AT_decl_column = 0x39,             // constant
    DW_AT_decl_file = 0x3a,               // constant
    DW_AT_decl_line = 0x3b,               // constant
    DW_AT_declaration = 0x3c,             // flag
    DW_AT_discr_list = 0x3d,              // block
    DW_AT_encoding = 0x3e,                // constant
    DW_AT_external = 0x3f,                // flag
    DW_AT_frame_base = 0x40,              // exprloc, loclist
    DW_AT_friend = 0x41,                  // reference
    DW_AT_identifier_case = 0x42,         // constant
    DW_AT_namelist_item = 0x44,           // reference
    DW_AT_priority = 0x45,                // reference
    DW_AT_segment = 0x46,                 // exprloc, loclist
    DW_AT_specification = 0x47,           // reference
    DW_AT_static_link = 0x48,             // exprloc, loclist
    DW_AT_type = 0x49,                    // reference
    DW_AT_use_location = 0x4a,            // exprloc, loclist
    DW_AT_variable_parameter = 0x4b,      // flag
    DW_AT_virtuality = 0x4c,              // constant
    DW_AT_vtable_elem_location = 0x4d,    // exprloc, loclist
    DW_AT_allocated = 0x4e,               // constant, exprloc, reference
    DW_AT_associated = 0x4f,              // constant, exprloc, reference
    DW_AT_data_location = 0x50,           // exprloc
    DW_AT_byte_stride = 0x51,             // constant, exprloc, reference
    DW_AT_entry_pc = 0x52,                // address, constant
    DW_AT_use_UTF8 = 0x53,                // flag
    DW_AT_extension = 0x54,               // reference
    DW_AT_ranges = 0x55,                  // rnglist
    DW_AT_trampoline = 0x56,              // address, flag, reference, string
    DW_AT_call_column = 0x57,             // constant
    DW_AT_call_file = 0x58,               // constant
    DW_AT_call_line = 0x59,               // constant
    DW_AT_description = 0x5a,             // string
    DW_AT_binary_scale = 0x5b,            // constant
    DW_AT_decimal_scale = 0x5c,           // constant
    DW_AT_small = 0x5d,                   // reference
    DW_AT_decimal_sign = 0x5e,            // constant
    DW_AT_digit_count = 0x5f,             // constant
    DW_AT_picture_string = 0x60,          // string
    DW_AT_mutable = 0x61,                 // flag
    DW_AT_threads_scaled = 0x62,          // flag
    DW_AT_explicit = 0x63,                // flag
    DW_AT_object_pointer = 0x64,          // reference
    DW_AT_endianity = 0x65,               // constant
    DW_AT_elemental = 0x66,               // flag
    DW_AT_pure = 0x67,                    // flag
    DW_AT_recursive = 0x68,               // flag
    DW_AT_signature = 0x69,               // reference
    DW_AT_main_subprogram = 0x6a,         // flag
    DW_AT_data_bit_offset = 0x6b,         // constant
    DW_AT_const_expr = 0x6c,              // flag
    DW_AT_enum_class = 0x6d,              // flag
    DW_AT_linkage_name = 0x6e,            // string
    DW_AT_string_length_bit_size = 0x6f,  // constant
    DW_AT_string_length_byte_size = 0x70, // constant
    DW_AT_rank = 0x71,                    // constant, exprloc
    DW_AT_str_offsets_base = 0x72,        // stroffsetsptr
    DW_AT_addr_base = 0x73,               // addrptr
    DW_AT_rnglists_base = 0x74,           // rnglistsptr
    DW_AT_dwo_name = 0x76,                // string
    DW_AT_reference = 0x77,               // flag
    DW_AT_rvalue_reference = 0x78,        // flag
    DW_AT_macros = 0x79,                  // macptr
    DW_AT_call_all_calls = 0x7a,          // flag
    DW_AT_call_all_source_calls = 0x7b,   // flag
    DW_AT_call_all_tail_calls = 0x7c,     // flag
    DW_AT_call_return_pc = 0x7d,          // address
    DW_AT_call_value = 0x7e,              // exprloc
    DW_AT_call_origin = 0x7f,             // exprloc
    DW_AT_call_parameter = 0x80,          // reference
    DW_AT_call_pc = 0x81,                 // address
    DW_AT_call_tail_call = 0x82,          // flag
    DW_AT_call_target = 0x83,             // exprloc
    DW_AT_call_target_clobbered = 0x84,   // exprloc
    DW_AT_call_data_location = 0x85,      // exprloc
    DW_AT_call_data_value = 0x86,         // exprloc
    DW_AT_noreturn = 0x87,                // flag
    DW_AT_alignment = 0x88,               // constant
    DW_AT_export_symbols = 0x89,          // flag
    DW_AT_deleted = 0x8a,                 // flag
    DW_AT_defaulted = 0x8b,               // constant
    DW_AT_loclists_base = 0x8c,           // loclistsptr
    DW_AT_lo_user = 0x2000,               // -
    DW_AT_hi_user = 0x3fff,               // -
} Dwarf_Attribute_Type;


static char *dwarf_dbg_attr_type(Dwarf_Attribute_Type type) {
    if(type == DW_AT_sibling) return "DW_AT_sibling";
    if(type == DW_AT_location) return "DW_AT_location";
    if(type == DW_AT_name) return "DW_AT_name";
    if(type == DW_AT_ordering) return "DW_AT_ordering";
    if(type == DW_AT_byte_size) return "DW_AT_byte_size";
    if(type == DW_AT_bit_size) return "DW_AT_bit_size";
    if(type == DW_AT_stmt_list) return "DW_AT_stmt_list";
    if(type == DW_AT_low_pc) return "DW_AT_low_pc";
    if(type == DW_AT_high_pc) return "DW_AT_high_pc";
    if(type == DW_AT_language) return "DW_AT_language";
    if(type == DW_AT_discr) return "DW_AT_discr";
    if(type == DW_AT_discr_value) return "DW_AT_discr_value";
    if(type == DW_AT_visibility) return "DW_AT_visibility";
    if(type == DW_AT_import) return "DW_AT_import";
    if(type == DW_AT_string_length) return "DW_AT_string_length";
    if(type == DW_AT_common_reference) return "DW_AT_common_reference";
    if(type == DW_AT_comp_dir) return "DW_AT_comp_dir";
    if(type == DW_AT_const_value) return "DW_AT_const_value";
    if(type == DW_AT_containing_type) return "DW_AT_containing_type";
    if(type == DW_AT_default_value) return "DW_AT_default_value";
    if(type == DW_AT_inline) return "DW_AT_inline";
    if(type == DW_AT_is_optional) return "DW_AT_is_optional";
    if(type == DW_AT_lower_bound) return "DW_AT_lower_bound";
    if(type == DW_AT_producer) return "DW_AT_producer";
    if(type == DW_AT_prototyped) return "DW_AT_prototyped";
    if(type == DW_AT_return_addr) return "DW_AT_return_addr";
    if(type == DW_AT_start_scope) return "DW_AT_start_scope";
    if(type == DW_AT_bit_stride) return "DW_AT_bit_stride";
    if(type == DW_AT_upper_bound) return "DW_AT_upper_bound";
    if(type == DW_AT_abstract_origin) return "DW_AT_abstract_origin";
    if(type == DW_AT_accessibility) return "DW_AT_accessibility";
    if(type == DW_AT_address_class) return "DW_AT_address_class";
    if(type == DW_AT_artificial) return "DW_AT_artificial";
    if(type == DW_AT_base_types) return "DW_AT_base_types";
    if(type == DW_AT_calling_convention) return "DW_AT_calling_convention";
    if(type == DW_AT_count) return "DW_AT_count";
    if(type == DW_AT_data_member_location) return "DW_AT_data_member_location";
    if(type == DW_AT_decl_column) return "DW_AT_decl_column";
    if(type == DW_AT_decl_file) return "DW_AT_decl_file";
    if(type == DW_AT_decl_line) return "DW_AT_decl_line";
    if(type == DW_AT_declaration) return "DW_AT_declaration";
    if(type == DW_AT_discr_list) return "DW_AT_discr_list";
    if(type == DW_AT_encoding) return "DW_AT_encoding";
    if(type == DW_AT_external) return "DW_AT_external";
    if(type == DW_AT_frame_base) return "DW_AT_frame_base";
    if(type == DW_AT_friend) return "DW_AT_friend";
    if(type == DW_AT_identifier_case) return "DW_AT_identifier_case";
    if(type == DW_AT_namelist_item) return "DW_AT_namelist_item";
    if(type == DW_AT_priority) return "DW_AT_priority";
    if(type == DW_AT_segment) return "DW_AT_segment";
    if(type == DW_AT_specification) return "DW_AT_specification";
    if(type == DW_AT_static_link) return "DW_AT_static_link";
    if(type == DW_AT_type) return "DW_AT_type";
    if(type == DW_AT_use_location) return "DW_AT_use_location";
    if(type == DW_AT_variable_parameter) return "DW_AT_variable_parameter";
    if(type == DW_AT_virtuality) return "DW_AT_virtuality";
    if(type == DW_AT_vtable_elem_location) return "DW_AT_vtable_elem_location";
    if(type == DW_AT_allocated) return "DW_AT_allocated";
    if(type == DW_AT_associated) return "DW_AT_associated";
    if(type == DW_AT_data_location) return "DW_AT_data_location";
    if(type == DW_AT_byte_stride) return "DW_AT_byte_stride";
    if(type == DW_AT_entry_pc) return "DW_AT_entry_pc";
    if(type == DW_AT_use_UTF8) return "DW_AT_use_UTF8";
    if(type == DW_AT_extension) return "DW_AT_extension";
    if(type == DW_AT_ranges) return "DW_AT_ranges";
    if(type == DW_AT_trampoline) return "DW_AT_trampoline";
    if(type == DW_AT_call_column) return "DW_AT_call_column";
    if(type == DW_AT_call_file) return "DW_AT_call_file";
    if(type == DW_AT_call_line) return "DW_AT_call_line";
    if(type == DW_AT_description) return "DW_AT_description";
    if(type == DW_AT_binary_scale) return "DW_AT_binary_scale";
    if(type == DW_AT_decimal_scale) return "DW_AT_decimal_scale";
    if(type == DW_AT_small) return "DW_AT_small";
    if(type == DW_AT_decimal_sign) return "DW_AT_decimal_sign";
    if(type == DW_AT_digit_count) return "DW_AT_digit_count";
    if(type == DW_AT_picture_string) return "DW_AT_picture_string";
    if(type == DW_AT_mutable) return "DW_AT_mutable";
    if(type == DW_AT_threads_scaled) return "DW_AT_threads_scaled";
    if(type == DW_AT_explicit) return "DW_AT_explicit";
    if(type == DW_AT_object_pointer) return "DW_AT_object_pointer";
    if(type == DW_AT_endianity) return "DW_AT_endianity";
    if(type == DW_AT_elemental) return "DW_AT_elemental";
    if(type == DW_AT_pure) return "DW_AT_pure";
    if(type == DW_AT_recursive) return "DW_AT_recursive";
    if(type == DW_AT_signature) return "DW_AT_signature";
    if(type == DW_AT_main_subprogram) return "DW_AT_main_subprogram";
    if(type == DW_AT_data_bit_offset) return "DW_AT_data_bit_offset";
    if(type == DW_AT_const_expr) return "DW_AT_const_expr";
    if(type == DW_AT_enum_class) return "DW_AT_enum_class";
    if(type == DW_AT_linkage_name) return "DW_AT_linkage_name";
    if(type == DW_AT_string_length_bit_size) return "DW_AT_string_length_bit_size";
    if(type == DW_AT_string_length_byte_size) return "DW_AT_string_length_byte_size";
    if(type == DW_AT_rank) return "DW_AT_rank";
    if(type == DW_AT_str_offsets_base) return "DW_AT_str_offsets_base";
    if(type == DW_AT_addr_base) return "DW_AT_addr_base";
    if(type == DW_AT_rnglists_base) return "DW_AT_rnglists_base";
    if(type == DW_AT_dwo_name) return "DW_AT_dwo_name";
    if(type == DW_AT_reference) return "DW_AT_reference";
    if(type == DW_AT_rvalue_reference) return "DW_AT_rvalue_reference";
    if(type == DW_AT_macros) return "DW_AT_macros";
    if(type == DW_AT_call_all_calls) return "DW_AT_call_all_calls";
    if(type == DW_AT_call_all_source_calls) return "DW_AT_call_all_source_calls";
    if(type == DW_AT_call_all_tail_calls) return "DW_AT_call_all_tail_calls";
    if(type == DW_AT_call_return_pc) return "DW_AT_call_return_pc";
    if(type == DW_AT_call_value) return "DW_AT_call_value";
    if(type == DW_AT_call_origin) return "DW_AT_call_origin";
    if(type == DW_AT_call_parameter) return "DW_AT_call_parameter";
    if(type == DW_AT_call_pc) return "DW_AT_call_pc";
    if(type == DW_AT_call_tail_call) return "DW_AT_call_tail_call";
    if(type == DW_AT_call_target) return "DW_AT_call_target";
    if(type == DW_AT_call_target_clobbered) return "DW_AT_call_target_clobbered";
    if(type == DW_AT_call_data_location) return "DW_AT_call_data_location";
    if(type == DW_AT_call_data_value) return "DW_AT_call_data_value";
    if(type == DW_AT_noreturn) return "DW_AT_noreturn";
    if(type == DW_AT_alignment) return "DW_AT_alignment";
    if(type == DW_AT_export_symbols) return "DW_AT_export_symbols";
    if(type == DW_AT_deleted) return "DW_AT_deleted";
    if(type == DW_AT_defaulted) return "DW_AT_defaulted";
    if(type == DW_AT_loclists_base) return "DW_AT_loclists_base";
    if(type == DW_AT_lo_user) return "DW_AT_lo_user";
    if(type == DW_AT_hi_user) return "DW_AT_hi_user";
    return "(unknown)";
}

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

    for (;;) {
        if (parse_eof(parse)) break;
        u64 abbrev_code = parse_uleb128(parse);
        if (abbrev_code == 0) break;
        if (parse_eof(parse)) break;
        fmt_sx(fout, "Abbrev: ", abbrev_code, "\n");
        u64 die_tag = parse_uleb128(parse);
        fmt_sx(fout, "DIE: ", die_tag, "\n");
        u8 has_children = parse_u8(parse);
        fmt_su(fout, "has_children: ", has_children, "\n");

        for (;;) {
            u64 attr_name = parse_uleb128(parse);
            u64 attr_form = parse_uleb128(parse);
            if (attr_name == 0 && attr_form == 0) break;
            fmt_s(fout, "Attr: ");
            fmt_s(fout, dwarf_dbg_attr_type(attr_name));
            fmt_s(fout, " ");
            fmt_x(fout, attr_form);
            fmt_s(fout, "\n");
            if (attr_form == DW_FORM_implicit_const) {
                i64 form_value = parse_ileb128(parse);
                fmt_si(fout, "Implicit const: ", form_value, "\n");
            }
        }
    }
}
