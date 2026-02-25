// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// deflate.h - DEFLATE decompressor implementation
#pragma once
#include "huffman_code.h"
#include "huffman_tree.h"

static bool deflate_write(Memory *mem, Buffer input, Buffer *output) {
    Memory *tmp = mem_new();

    // First encode all data directly as symbols
    u32 symbol_count = input.size + 1;
    u16 *symbols = mem_array(tmp, u16, symbol_count);

    // Add normal values
    for (u32 i = 0; i < input.size; ++i) {
        symbols[i] = input.data[i];
    }

    // End of block marker
    symbols[symbol_count - 1] = 256;

    // Create frequency list
    u32 freq_list[288] = {};
    for (u32 i = 0; i < symbol_count; ++i) {
        freq_list[symbols[i]]++;
    }

    // Construct huffman tree (limited to 15 bits)
    Huffman_Tree *tree = huffman_tree_from_length_limited(tmp, array_count(freq_list), freq_list, 15);
    try(tree);

    u8 len_list[288] = {};
    try(huffman_tree_to_lengths(tree, array_count(len_list), len_list));
    Huffman_Code *code = huffman_code_from(tmp, array_count(len_list), len_list);
    try(code);

    mem_free(tmp);
    return ok();
}
