// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// deflate_huffman.h - DEFLATE decompressor implementation
#pragma once
#include "huffman_code.h"
#include "huffman_tree.h"
#include "mem.h"
#include "stream.h"
#include "type.h"

// Combination of length and distance symbol huffman codes
typedef struct {
    Huffman_Code *length;
    Huffman_Code *distance;
} Deflate_Huffman;

// Accumulator that counts the number of occurrences of each symbol
// This can be used to construct the dynamic huffman tables
typedef struct {
    u32 length_freq[288];
    u32 distance_freq[30];
} Deflate_Encode_Info;

// Create the fixed huffman table for block type 1
static Deflate_Huffman *deflate_huffman_fixed(Memory *mem) {
    Deflate_Huffman *tab = mem_struct(mem, Deflate_Huffman);

    // length prefix code bit sizes
    u8 h_len_bits[288];
    for (u32 i = 0; i < 144; ++i) h_len_bits[i] = 8;
    for (u32 i = 144; i < 256; ++i) h_len_bits[i] = 9;
    for (u32 i = 256; i < 280; ++i) h_len_bits[i] = 7;
    for (u32 i = 280; i < 288; ++i) h_len_bits[i] = 8;
    Huffman_Code *h_len = huffman_code_from(mem, array_count(h_len_bits), h_len_bits);

    // distance prefix code bit sizes are always 5
    u8 h_dist_bits[32];
    for (u32 i = 0; i < 32; ++i) h_dist_bits[i] = 5;
    Huffman_Code *h_dist = huffman_code_from(mem, array_count(h_dist_bits), h_dist_bits);

    tab->length = h_len;
    tab->distance = h_dist;
    return tab;
}

// Create a dynamic huffman table based on the symbol frequencies
static Deflate_Huffman *deflate_huffman_dynamic_create(Memory *mem, Deflate_Encode_Info *info) {
    // Construct an improved huffman code using the frequencies
    u8 length_bits[array_count(info->length_freq)] = {};
    u8 distance_bits[array_count(info->distance_freq)] = {};

    try(huffman_tree_freq_to_lengths(array_count(info->length_freq), info->length_freq, length_bits, 15));
    try(huffman_tree_freq_to_lengths(array_count(info->distance_freq), info->distance_freq, distance_bits, 15));

    Huffman_Code *length_code = huffman_code_from(mem, array_count(length_bits), length_bits);
    try(length_code);

    Huffman_Code *distance_code = huffman_code_from(mem, array_count(distance_bits), distance_bits);
    try(distance_code);

    Deflate_Huffman *table = mem_struct(mem, Deflate_Huffman);
    table->length = length_code;
    table->distance = distance_code;
    return table;
}

// Read the dynamic huffman table for block type 2 from the input stream
static Deflate_Huffman *deflate_huffman_dynamic_read(Memory *mem, Stream *input) {
    u32 length_count = stream_read_bits(input, 5) + 257;
    assert(length_count <= 288);

    u32 distance_count = stream_read_bits(input, 5) + 1;
    assert(distance_count <= 30);

    // CL codes
    u32 code_count = stream_read_bits(input, 4) + 4; // Number of length codes
    assert(code_count <= 19);

    u8 code_index[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    u8 code_lengths[19] = {0};
    for (u32 i = 0; i < code_count; ++i) {
        code_lengths[code_index[i]] = stream_read_bits(input, 3);
    }

    Huffman_Code *code_tree = huffman_code_from(mem, 19, code_lengths);

    u32 count = 0;
    u8 lengths[288 + 30];
    while (count < length_count + distance_count) {
        u32 symbol = huffman_code_read(code_tree, input);
        assert(symbol < 19);
        u8 repeat = 1;
        u8 length = symbol;

        // Copy previous value 3 to 6 times
        if (symbol == 16) {
            length = lengths[count - 1];
            repeat = stream_read_bits(input, 2) + 3;
        }

        // Repeat 0 value 3 to 10 times
        if (symbol == 17) {
            length = 0;
            repeat = stream_read_bits(input, 3) + 3;
        }

        // Repeat 0 value 11 - 138 times
        if (symbol == 18) {
            length = 0;
            repeat = stream_read_bits(input, 7) + 11;
        }

        for (u8 i = 0; i < repeat; ++i) lengths[count++] = length;
    }

    Deflate_Huffman *huffman = mem_struct(mem, Deflate_Huffman);
    huffman->length = huffman_code_from(mem, length_count, lengths);
    huffman->distance = huffman_code_from(mem, distance_count, lengths + length_count);
    return huffman;
}

static bool deflate_huffman_dynamic_write(Deflate_Huffman *code, Stream *output) {
    u32 length_count = 288;
    assert(length_count >= 257 && length_count <= 288);
    stream_write_bits(output, 5, length_count - 257);

    u32 distance_count = 30;
    assert(distance_count >= 1 && distance_count <= 30);
    stream_write_bits(output, 5, distance_count - 1);

    // CL codes

    u32 code_freq[19] = {};
    for (u32 i = 0; i < length_count; ++i) code_freq[code->length->symbol_len[i]]++;
    for (u32 i = 0; i < distance_count; ++i) code_freq[code->distance->symbol_len[i]]++;

    Memory *tmp = mem_new();

    u8 code_len[19] = {};
    try(huffman_tree_freq_to_lengths(array_count(code_freq), code_freq, code_len, 7));
    Huffman_Code *code_tree = huffman_code_from(tmp, array_count(code_len), code_len);
    try(code_tree);

    // Count codes
    u8 code_index[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    u32 code_count = 19;
    assert(code_count >= 4 && code_count <= 19);
    stream_write_bits(output, 4, code_count - 4);

    for (u32 i = 0; i < code_count; ++i) {
        stream_write_bits(output, 3, code_len[code_index[i]]);
    }

    for (u32 i = 0; i < length_count; ++i) {
        if (code->length->symbol_len[i]) fmt_su(fout, "leng: ", i, "\n");
        try(huffman_code_write(code_tree, output, code->length->symbol_len[i]));
    }

    for (u32 i = 0; i < distance_count; ++i) {
        if (code->distance->symbol_len[i]) fmt_su(fout, "dist: ", i, "\n");
        try(huffman_code_write(code_tree, output, code->distance->symbol_len[i]));
    }
    mem_free(tmp);
    return ok();
}
