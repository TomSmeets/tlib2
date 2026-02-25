// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// deflate_write.h - DEFLATE compression implementation
#pragma once
#include "huffman_code.h"
#include "huffman_tree.h"
#include "deflate.h"

typedef struct {
    u32 length_freq[288];
    u32 distance_freq[30];
} Deflate_Encode_Info;

static bool deflate_encode_lzss(Deflate_Huffman *code, Buffer input, Stream *output, Deflate_Encode_Info*info) {

    // Add values
    for (size_t i = 0; i < input.size; ++i) {
        // Decide what symbol to encode
        u16 symbol = input.data[i];

        // Write length symbol
        huffman_code_write(code->length, output, symbol);
        if (info) info->length_freq[symbol]++;
    }

    // End of block marker
    huffman_code_write(code->length, output, 256);
    if (info) info->length_freq[256]++;
    return ok();
}

static Deflate_Huffman *deflate_create_dynamic_huffman_from_info(Memory *mem, Deflate_Encode_Info *info, size_t max_lookback) {
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


typedef struct {
    // Which block types to allow
    bool enable_dynamic;
    bool enable_fixed;
    bool enable_stored;

    // How far to look backwards for repeated data
    size_t lzss_lookback;
} Deflate_Option;

static bool deflate_write(Memory *mem, Buffer input, Deflate_Option *opt, Buffer *output) {
    // Idea to reduce memory usage:
    // 1. Encode directly using fixed huffman table, and count freqs directly
    // 3. Re-encode the data using new huffman table
    // 
    // Temporary memory used during the encoding
    // Memory will be cleared at the end of this function
    
    Deflate_Encode_Info frequency_info = {};
    if (opt->enable_dynamic || opt->enable_fixed) {
        // Encode using fixed huffman code
        Stream *fixed_stream = stream_new(mem);
        Deflate_Huffman *fixed_code = deflate_create_fixed_huffman(mem);
        try(fixed_code);
        try(deflate_encode_lzss(fixed_code, input, fixed_stream, &frequency_info));
        *output = stream_to_buffer(fixed_stream);
    }

    if(opt->enable_dynamic) {
        // Try again using dynamic huffman code
        Stream *dynamic_stream = stream_new(mem);
        Deflate_Huffman *dynamic_code = deflate_create_dynamic_huffman_from_info(mem, &frequency_info, opt->lzss_lookback);
        try(dynamic_code);
        // TODO: encode huffman code table
        try(deflate_encode_lzss(dynamic_code, input, dynamic_stream, 0));

        if (!opt->enable_fixed || dynamic_stream->size < output->size) {
            *output = stream_to_buffer(dynamic_stream);
        }
    }

    if (opt->enable_stored) {
        // Still too big, just encode raw bytes
        size_t stored_block_count = (input.size / 0xffff) + 1;
        size_t stored_block_overhead = 1 + 2 + 2;
        size_t stored_size = input.size + stored_block_count * stored_block_overhead;
        if (output->size > stored_size) {
            Stream *stored_stream = stream_new(mem);
            stream_reserve(stored_stream, stored_size);

            size_t input_offset = 0;
            for(;;) {
                u16 block_size = MIN(input.size - input_offset, 0xffff);
                bool is_last = input_offset + block_size == input.size;
                stream_write_u8(stored_stream, is_last);
                stream_write_u16(stored_stream, block_size);
                stream_write_u16(stored_stream, block_size ^ 0xffff);
                stream_write_bytes(stored_stream, block_size, input.data + input_offset);
                input_offset += block_size;
                if(is_last) break;
            }
            *output = stream_to_buffer(stored_stream);
        }
    }
    return ok();
}

static bool deflate_write_test(void) {
    Buffer input = str_buf("hello world hello world hello world!");
    Buffer output = {};
    Memory *mem = mem_new();
    fmt_hexdump(fout, input);

    Deflate_Option opt = {
        .enable_dynamic = 1,
        .enable_fixed = 1,
        .enable_stored = 1,
        .lzss_lookback = 64,
    };
    try(deflate_write(mem, input, &opt, &output));

    fmt_hexdump(fout, output);
    try(output.size > 0 && output.size < input.size);
    return ok();
}
