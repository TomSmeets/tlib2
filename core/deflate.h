// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// deflate.h - DEFLATE decompressor implementation
#pragma once
#include "deflate_huffman.h"
#include "deflate_llcode.h"
#include "deflate_lzss.h"
#include "huffman_code.h"
#include "huffman_tree.h"
#include "mem.h"
#include "os.h"
#include "stream.h"
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

static bool deflate_read(Memory *mem, Stream *input, Stream *output) {
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
    return true;
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

static bool deflate_write(Memory *mem, Buffer input, Buffer *output) {
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
    Stream *output_fixed = stream_new(mem);
    stream_write_bits(output_fixed, 1, 1);
    stream_write_bits(output_fixed, 2, Deflate_BlockFixed);
    try(deflate_lzss_encode(mem, fixed_code, input, output_fixed, &frequency_info));

    Deflate_Huffman *dynamic_code = deflate_huffman_dynamic_create(mem, &frequency_info);

    // Re encode with the new huffman table
    Stream *output_dynamic = stream_new(mem);
    stream_write_bits(output_dynamic, 1, 1);
    stream_write_bits(output_dynamic, 2, Deflate_BlockDynamic);
    try(deflate_huffman_dynamic_write(dynamic_code, output_dynamic));
    try(deflate_lzss_recode(mem, llcode, fixed_code, stream_to_buffer(output_fixed), dynamic_code, output_dynamic));

    // Check if dynamic data is smaller
    *output = stream_to_buffer(output_fixed);
    // if (output_dynamic->size < output->size) {
        *output = stream_to_buffer(output_dynamic);
    // }

    // Check if stored data is smaller
    // if (deflate_calculate_stored_block_size(input.size) <= output->size) {
        // deflate_write_stored(mem, input, output);
    // }

    return ok();
}

static bool deflate_read_buf(Memory *mem, Buffer input_buffer, Buffer *output_buffer) {
    Stream input = stream_from(input_buffer);
    Stream *output = stream_new(mem);
    try(deflate_read(mem, &input, output));
    *output_buffer = stream_to_buffer(output);
    return ok();
}

static bool deflate_test(void) {
    Buffer input = str_buf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    Memory *mem = mem_new();

    fmt_s(fout, "Input:\n");
    fmt_hexdump(fout, input);

    fmt_s(fout,"Compressed:\n");
    Buffer compressed   = {};
    try(deflate_write(mem, input, &compressed));
    fmt_hexdump(fout, compressed);

    fmt_s(fout,"Decompressed:\n");
    Buffer decompressed = {};
    try(deflate_read_buf(mem, compressed, &decompressed));
    fmt_hexdump(fout, decompressed);

    try(decompressed.size == input.size);
    try(mem_eq(decompressed.data, input.data, input.size));
    return ok();
}
