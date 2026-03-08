// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// deflate.h - DEFLATE decompressor implementation
#pragma once
#include "deflate_huffman.h"
#include "deflate_llcode.h"
#include "deflate_lz77.h"
#include "huffman_code.h"
#include "huffman_tree.h"
#include "mem.h"
#include "os.h"
#include "stream.h"
#include "rand.h"
#include "type.h"

// Deflate block types (2 bit value)
typedef enum {
    Deflate_BlockStored = 0,  // Raw uncompressed data
    Deflate_BlockFixed = 1,   // Fixed Huffman table + LZ77
    Deflate_BlockDynamic = 2, // Dynamic Huffman table + LZ77
} Deflate_BlockType;

static size_t deflate_calculate_stored_block_size(size_t input_size) {
    size_t stored_block_count = (input_size / 0xffff) + 1;
    size_t stored_block_overhead = 1 + 2 + 2;
    size_t stored_size = input_size + stored_block_count * stored_block_overhead;
    return stored_size;
}

static bool deflate_read(Memory *mem, Buffer input_buf, Buffer *output_buf) {
    Stream input_ = stream_from(input_buf);
    Stream *input = &input_;

    Stream *output = stream_new(mem);
    for (;;) {
        try(stream_eof(input) == false);

        // Read block header
        bool is_last = stream_read_bits(input, 1);
        Deflate_BlockType type = stream_read_bits(input, 2);

        if (type == Deflate_BlockStored) {
            // This block is stored directly, no special encoding
            u16 size = stream_read_u16(input);
            u16 size_check = stream_read_u16(input);
            try(size == (size_check ^ 0xffff));

            for (size_t i = 0; i < size; ++i) {
                stream_write_u8(output, stream_read_u8(input));
            }
        } else {
            // This is an Huffman + LZ77 encoded block

            // Construct length and distance code lookup table
            Deflate_LLCode *code = deflate_llcode_new(mem);
            Deflate_Huffman *tree = 0;

            // Fixed blocks have a pre-defined huffman tree
            if (type == Deflate_BlockFixed) tree = deflate_huffman_fixed(mem);

            // Dynamic blocks store the huffman tree at the beginning of the stream
            if (type == Deflate_BlockDynamic) tree = deflate_huffman_dynamic_read(mem, input);

            // Tree should be valid
            try(tree);

            while (1) {
                // Read encoded length-symbol using the huffman tree
                u32 symbol = huffman_code_read(tree->length, input);

                // Symbol must be valid (return 0 otherwise)
                try(symbol < 288);

                // End of block marker
                if (symbol == 256) break;

                // A regular byte
                if (symbol < 256) stream_write_u8(output, symbol);

                // A LZ77 sequence
                if (symbol > 256) {
                    // Symbols 257-287 encode the length of the back reference
                    // with a few extra bits depending on the symbol
                    u32 length_code = symbol - 257;
                    u32 length = deflate_llcode_length_read(code, input, length_code);

                    // The distance is how far to look backwards, these are encoded
                    // using their own huffman tree, and are also followed by a few extra bits
                    u32 distance_code = huffman_code_read(tree->distance, input);
                    u32 distance = deflate_llcode_distance_read(code, input, distance_code);

                    // Look backwards and emit the bytes in order
                    size_t cursor = output->cursor - distance;
                    for (size_t i = 0; i < length; ++i) {
                        u8 c = output->buffer[cursor + i];
                        stream_write_u8(output, c);
                    }
                }
            }
        }

        // Continue reading until the last block
        if (is_last) break;
    }
    *output_buf = stream_to_buffer(output);
    return ok();
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

typedef struct {
    size_t size;
    u8 *data[];
} Bytes;

typedef struct {
    bool ok;
    Buffer output;
    Deflate_Encode_Info frequency_info;
    Deflate_Huffman *encoding;
} Deflate_Result;

static bool
deflate_write_fixed(Memory *mem, Deflate_LLCode *llcode, Deflate_Huffman *code, Buffer input, Buffer *result, Deflate_Encode_Info *frequency_info) {
    Stream *stream = stream_new(mem);
    stream_write_bits(stream, 1, 1);
    stream_write_bits(stream, 2, Deflate_BlockFixed);
    try(deflate_lz_encode(mem, code, llcode, input, stream, frequency_info));
    *result = stream_to_buffer(stream);
    return ok();
}

static bool
deflate_write_dynamic(Memory *mem, Deflate_LLCode *llcode, Deflate_Huffman *input_code, Buffer input, Deflate_Huffman *output_code, Buffer *output) {
    Stream *stream = stream_new(mem);
    stream_write_bits(stream, 1, 1);
    stream_write_bits(stream, 2, Deflate_BlockDynamic);
    try(deflate_huffman_dynamic_write(output_code, stream));
    try(deflate_lz_recode(mem, llcode, input_code, input, output_code, stream));
    *output = stream_to_buffer(stream);
    return ok();
}

static bool deflate_write(Memory *mem, Buffer input, Buffer *result) {
    bool enable_stored = 1;
    bool enable_dynamic = 1;
    bool enable_fixed = 1;
    // Idea to reduce memory usage:
    // 1. Encode directly using fixed huffman table, and count freqs directly
    // 3. Re-encode the data using new huffman table
    //
    // Temporary memory used during the encoding
    // Memory will be cleared at the end of this function

    // Symbol frequency info

    // Fixed huffman table

    // Length/Distnace Symbol to offset/bit_count mapping
    Deflate_LLCode *llcode = deflate_llcode_new(mem);
    *result = (Buffer){};

    // Encode using fixed huffman code and also collect frequency info
    Deflate_Huffman *fixed_code = deflate_huffman_fixed(mem);
    try(fixed_code);

    Deflate_Encode_Info frequency_info = {};
    Buffer fixed_output = {};
    try(deflate_write_fixed(mem, llcode, fixed_code, input, &fixed_output, &frequency_info));
    if (enable_fixed) *result = fixed_output;

    if (enable_dynamic) {
        // Re encode with the new huffman table
        Deflate_Huffman *dynamic_code = deflate_huffman_dynamic_create(mem, &frequency_info);
        try(dynamic_code);

        Buffer dynamic_output = {};
        try(deflate_write_dynamic(mem, llcode, fixed_code, fixed_output, dynamic_code, &dynamic_output));
        if (result->size == 0 || result->size > dynamic_output.size) *result = dynamic_output;
    }

    if (enable_stored && (result->size == 0 || result->size > deflate_calculate_stored_block_size(input.size))) {
        Buffer stored_output = {};
        try(deflate_write_stored(mem, input, &stored_output));
        *result = stored_output;
    }

    return ok();
}

// Run a deflate/inflate testcase with a given input
static bool deflate_test_buf(Memory *mem, Buffer input) {
    print("Input:\n", input);

    Buffer compressed = {};
    try(deflate_write(mem, input, &compressed));
    print("Compressed:\n", compressed);

    Buffer decompressed = {};
    try(deflate_read(mem, compressed, &decompressed));
    print("Decompressed:\n", decompressed);

    try(buf_eq(decompressed, input));
    return ok();
}

static bool deflate_test(void) {
    {
        Memory *mem = mem_new();
        try(deflate_test_buf(mem, str_buf("heeeeeeeeeeeeello hello")));
        try(deflate_test_buf(mem, str_buf("Hello World!")));
        try(deflate_test_buf(mem, str_buf("1234567")));
        try(deflate_test_buf(mem, str_buf("")));
        try(deflate_test_buf(mem, str_buf("0")));
        try(deflate_test_buf(mem, str_buf("0000000000000000")));
        try(deflate_test_buf(mem, str_buf("0000000000000001")));
        try(deflate_test_buf(mem, str_buf("1000000000000001")));
        mem_free(mem);
    }

    Rand rng = {};
    for (u32 i = 0; i < 16; ++i) {
        Memory *mem = mem_new();
        size_t input_size = 1 << i;
        Buffer input = {mem_alloc_zero(mem, input_size), input_size};
        try(deflate_test_buf(mem, input));
        rand_bytes(&rng, input);
        try(deflate_test_buf(mem, input));
        mem_free(mem);
    }
    return ok();
}
