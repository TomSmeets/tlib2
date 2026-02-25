// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// deflate_write.h - DEFLATE compression implementation
#pragma once
#include "huffman_code.h"
#include "huffman_tree.h"

static bool deflate_write(Memory *mem, Buffer input, Buffer *output) {
    // Temporary memory used during the encoding
    // Memory will be cleared at the end of this function
    Memory *tmp = mem_new();

    // First encode all data directly as symbols
    u32 symbol_count = input.size + 1;
    u16 *symbols = mem_array(tmp, u16, symbol_count);

    // Add normal values
    for (u32 i = 0; i < input.size; ++i) {
        symbols[i] = input.data[i];
    }

    // End of block marker
    symbols[input.size] = 256;

    // Create frequency list
    u32 freq_list[288] = {};
    for (u32 i = 0; i < symbol_count; ++i) {
        freq_list[symbols[i]]++;
    }

    // Construct list of bit lengths
    u8 len_list[288] = {};
    try(huffman_tree_freq_to_lengths(array_count(len_list), freq_list, len_list, 15));

    if (1) {
        // Debug info
        for (u32 i = 0; i < array_count(len_list); ++i) {
            if (!len_list[i]) continue;
            fmt_u_ex(fout, i, 10, ' ', 4);
            fmt_s(fout, ": ");
            fmt_u(fout, len_list[i]);
            fmt_s(fout, " bits");
            if (chr_is_printable(i)) {
                fmt_s(fout, " ('");
                fmt_c(fout, i);
                fmt_s(fout, "')");
            }
            fmt_s(fout, "\n");
        }
    }

    Huffman_Code *code = huffman_code_from(tmp, array_count(len_list), len_list);
    try(code);

    mem_free(tmp);
    return ok();
}

static bool deflate_write_test(void) {
    Buffer input = str_buf("hello world hello world hello world!");
    Buffer output = {};
    Memory *mem = mem_new();
    try(deflate_write(mem, input, &output));
    return ok();
}
