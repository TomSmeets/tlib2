// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// deflate_write.h - DEFLATE compression implementation
#pragma once
#include "deflate.h"
#include "huffman_code.h"
#include "huffman_tree.h"

typedef struct {
    // Which block types to allow
    bool enable_dynamic;
    bool enable_fixed;
    bool enable_stored;

    // How far to look backwards for repeated data
    size_t lzss_lookback;
} Deflate_Option;

static size_t deflate_calculate_stored_block_size(size_t input_size) {
    size_t stored_block_count = (input_size / 0xffff) + 1;
    size_t stored_block_overhead = 1 + 2 + 2;
    size_t stored_size = input_size + stored_block_count * stored_block_overhead;
    return stored_size;
}

static bool deflate_write_stored(Memory *mem, Buffer input, Buffer *output) {
    size_t stored_size = deflate_calculate_stored_block_size(input.size);
    Stream *stored_stream = stream_new(mem);
    try(stream_reserve(stored_stream, stored_size));

    size_t input_offset = 0;
    for (;;) {
        u16 block_size = MIN(input.size - input_offset, 0xffff);
        bool is_last = input_offset + block_size == input.size;
        stream_write_u8(stored_stream, is_last);
        stream_write_u16(stored_stream, block_size);
        stream_write_u16(stored_stream, block_size ^ 0xffff);
        stream_write_bytes(stored_stream, block_size, input.data + input_offset);
        input_offset += block_size;
        if (is_last) break;
    }
    *output = stream_to_buffer(stored_stream);
    return ok();
}

static bool
deflate_lzss_recode(Memory *mem, Deflate_LLCode *llcode, Deflate_Huffman *input_code, Buffer input, Deflate_Huffman *output_code, Buffer *output) {
    Stream input_stream = stream_from(input);
    Stream *output_stream = stream_new(mem);

    for (;;) {
        u32 symbol = huffman_code_read(input_code->length, &input_stream);
        huffman_code_write(output_code->length, output_stream, symbol);

        if (symbol == 256) break;
        if (symbol < 256) continue;
        try(symbol < 288);

        // Construct length symbol
        u32 length_symbol = symbol - 256;
        try(length_symbol < array_count(llcode->length_bits));

        // Copy length bits
        u32 length_bits = llcode->length_bits[length_symbol];
        for (u32 i = 0; i < length_bits; ++i) stream_write_bit(output_stream, stream_read_bit(&input_stream));

        // Re-encode distance symbol
        u32 distance_symbol = huffman_code_read(input_code->distance, &input_stream);
        huffman_code_write(output_code->distance, output_stream, distance_symbol);

        // Copy distance bits
        try(distance_symbol < array_count(llcode->distance_bits));
        u32 distance_bits = llcode->distance_bits[distance_symbol];
        for (u32 i = 0; i < distance_bits; ++i) stream_write_bit(output_stream, stream_read_bit(&input_stream));
    }

    *output = stream_to_buffer(output_stream);
    return ok();
}

static bool deflate_write(Memory *mem, Buffer input, Deflate_Option *opt, Buffer *output) {
    // Idea to reduce memory usage:
    // 1. Encode directly using fixed huffman table, and count freqs directly
    // 3. Re-encode the data using new huffman table
    //
    // Temporary memory used during the encoding
    // Memory will be cleared at the end of this function

    // Symbol frequency info
    Deflate_Encode_Info frequency_info = {};

    // Fixed huffman table
    Deflate_Huffman *fixed_code = deflate_huffman_fixed(mem);

    // Length/Distnace Symbol to offset/bit_count mapping
    Deflate_LLCode *llcode = deflate_llcode_new(mem);

    // Encode using fixed huffman code and also collect frequency info
    Buffer fixed_output = {};
    try(deflate_lzss_encode(mem, fixed_code, input, &fixed_output, &frequency_info));

    // Re encode with the new huffman table
    Deflate_Huffman *dynamic_code = deflate_huffman_dynamic_create(mem, &frequency_info);
    Buffer dynamic_output = {};
    // TODO: encode dynamic header
    try(deflate_lzss_recode(mem, llcode, fixed_code, fixed_output, dynamic_code, &dynamic_output));

    // Check if dynamic data is smaller
    *output = fixed_output;
    if (dynamic_output.size < output->size) *output = dynamic_output;

    // Check if stored data is smaller
    if (deflate_calculate_stored_block_size(input.size) <= output->size) {
        deflate_write_stored(mem, input, output);
    }

    return ok();
}

static bool deflate_write_test(void) {
    Buffer input = str_buf("hello world hello world hello world!");
    Buffer output = {};
    Memory *mem = mem_new();
    fmt_hexdump(fout, input);

    Deflate_Option opt = {
        .enable_dynamic = 0,
        .enable_fixed = 1,
        .enable_stored = 0,
        .lzss_lookback = 64,
    };
    try(deflate_write(mem, input, &opt, &output));

    fmt_hexdump(fout, output);
    try(output.size > 0);
    try(output.size < input.size);
    return ok();
}
