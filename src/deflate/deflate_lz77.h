// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// deflate.h - DEFLATE decompressor implementation
#pragma once
#include "deflate_huffman.h"
#include "deflate_llcode.h"

// Find index into data containing the prefix sequence of key
static Buffer buf_find_longest_match(Buffer data, size_t start, size_t max_distance) {
    size_t match_index = 0;
    size_t match_size = 0;
    if (max_distance > start) max_distance = start;
    Buffer a = buf_drop(data, start);
    for (size_t i = 0; i < max_distance; ++i) {
        size_t j = start - i - 1;
        Buffer b = buf_drop(data, j);
        size_t len = buf_match_len(a, b);
        if (len <= match_size) continue;
        match_size = len;
        match_index = j;
        // max length
        if (len >= 258) break;
    }
    return buf_from(data.data + match_index, match_size);
}

static void deflate_llcode_length_write(Write *out, Deflate_Huffman *tree, Deflate_LLCode *code, u32 length, Deflate_Encode_Info *info) {
    for (u32 i = 0; i < array_count(code->length_bits); ++i) {
        u32 start = code->length_offset[i];
        u32 bits = code->length_bits[i];
        u32 count = 1 << bits;
        if (i == 27) count--;
        if (length >= start && length < start + count) {
            u32 symbol = i + 257;
            if (info) info->length_freq[symbol]++;
            huffman_code_write(tree->length, out, symbol);
            write_bits(out, bits, length - start);
            return;
        }
    }
    check(!"Not Possible");
}

static void deflate_llcode_distance_write(Write *out, Deflate_Huffman *tree, Deflate_LLCode *code, u32 distance, Deflate_Encode_Info *info) {
    for (u32 i = 0; i < array_count(code->distance_bits); ++i) {
        u32 start = code->distance_offset[i];
        u32 bits = code->distance_bits[i];
        u32 count = 1 << bits;
        if (distance >= start && distance < start + count) {
            u32 symbol = i;
            if (info) info->distance_freq[symbol]++;
            huffman_code_write(tree->distance, out, symbol);
            write_bits(out, bits, distance - start);
            return;
        }
    }
    check(!"Not Possible");
}

// Compress the input data using LZ77 and encode it with the provided huffman code
// The symbol frequencies in the optional argument 'info' are incremented if present
static void deflate_lz_encode(Memory *mem, Deflate_Huffman *code, Deflate_LLCode *ll, Buffer input, Write *write, Deflate_Encode_Info *info) {
    // Add values
    for (size_t i = 0; i < input.size; ++i) {
        Buffer match = buf_find_longest_match(input, i, 1 << 15);
        if (match.size <= 3) {
            // Raw symbol
            u16 symbol = input.data[i];
            huffman_code_write(code->length, write, symbol);
            if (info) info->length_freq[symbol]++;
        } else {
            // LZ77 sequence
            u32 match_distance = input.data - match.data + i;
            u32 match_len = match.size;
            u32 max_len = 258;
            if (match_len > max_len) match_len = max_len;
            deflate_llcode_length_write(write, code, ll, match_len, info);
            deflate_llcode_distance_write(write, code, ll, match_distance, info);
            // print("Match: ", match_distance, " ", match_len);
            i += match_len - 1;
        }
    }

    // End of block marker
    huffman_code_write(code->length, write, 256);
    if (info) info->length_freq[256]++;
}

static void
deflate_lz_recode(Memory *mem, Deflate_LLCode *llcode, Deflate_Huffman *input_code, Buffer input, Deflate_Huffman *output_code, Write *write) {
    Read read = read_from(input);
    read_bits(&read, 3);

    for (;;) {
        u32 symbol = huffman_code_read(input_code->length, &read);
        huffman_code_write(output_code->length, write, symbol);

        if (symbol == 256) break;
        if (symbol < 256) continue;
        check_or(symbol < 288) return;

        // Construct length symbol
        u32 length_symbol = symbol - 257;
        check_or(length_symbol < array_count(llcode->length_bits)) return;

        // Copy length bits
        u32 length_bits = llcode->length_bits[length_symbol];
        for (u32 i = 0; i < length_bits; ++i) write_bit(write, read_bit(&read));

        // Re-encode distance symbol
        u32 distance_symbol = huffman_code_read(input_code->distance, &read);
        huffman_code_write(output_code->distance, write, distance_symbol);

        // Copy distance bits
        check_or(distance_symbol < array_count(llcode->distance_bits)) return;
        u32 distance_bits = llcode->distance_bits[distance_symbol];
        for (u32 i = 0; i < distance_bits; ++i) write_bit(write, read_bit(&read));
    }
}
