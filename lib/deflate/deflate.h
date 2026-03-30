// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// deflate.h - DEFLATE decompressor implementation
#pragma once
#include "deflate_huffman.h"
#include "deflate_llcode.h"
#include "deflate_lz77.h"
#include "read.h"
#include "huffman_code.h"
#include "huffman_tree.h"
#include "mem.h"
#include "os.h"
#include "write.h"
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

static Buffer deflate_read_from(Memory *mem, Read *input) {
    Write *output = write_new(mem);

    for (;;) {
        check(read_eof(input) == false);
        if (error) return buf_null();

        // Read block header
        bool is_last = read_bits(input, 1);
        Deflate_BlockType type = read_bits(input, 2);

        if (type == Deflate_BlockStored) {
            // This block is stored directly, no special encoding
            u16 size = read_u16(input);
            u16 size_check = read_u16(input);

            check(size == (size_check ^ 0xffff));
            if (error) return buf_null();

            for (size_t i = 0; i < size; ++i) {
                write_u8(output, read_u8(input));
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
            check(tree);

            while (1) {
                check(!read_bit_eof(input));
                if (error) return buf_null();

                // Read encoded length-symbol using the huffman tree
                u32 symbol = huffman_code_read(tree->length, input);

                // Symbol must be valid (return 0 otherwise)
                check(symbol < 288);
                if (error) return buf_null();

                // End of block marker
                if (symbol == 256) break;

                // A regular byte
                if (symbol < 256) write_u8(output, symbol);

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
                    write_repeat(output, distance, length);
                }
            }
        }

        // Continue reading until the last block
        if (is_last) break;
    }

    return write_get_written(output);
}

static Buffer deflate_read(Memory *mem, Buffer input) {
    Read read = read_from(input);
    return deflate_read_from(mem, &read);
}

static Buffer deflate_write_stored(Memory *mem, Buffer input) {
    size_t stored_size = deflate_calculate_stored_block_size(input.size);
    Write *stored_write = write_new(mem);
    write_reserve(stored_write, stored_size);
    if (error) return buf_null();

    size_t input_offset = 0;
    for (;;) {
        u16 block_size = MIN(input.size - input_offset, 0xffff);
        bool is_last = input_offset + block_size == input.size;
        write_u8(stored_write, is_last);
        write_u16(stored_write, block_size);
        write_u16(stored_write, block_size ^ 0xffff);
        write_buffer(stored_write, buf_slice(input, input_offset, block_size));
        input_offset += block_size;
        if (is_last) break;
    }
    return write_get_written(stored_write);
}

static Buffer deflate_write_fixed(Memory *mem, Deflate_LLCode *llcode, Deflate_Huffman *code, Buffer input, Deflate_Encode_Info *frequency_info) {
    Write *write = write_new(mem);
    write_bits(write, 1, 1);
    write_bits(write, 2, Deflate_BlockFixed);
    deflate_lz_encode(mem, code, llcode, input, write, frequency_info);
    return write_get_written(write);
}

static Buffer deflate_write_dynamic(Memory *mem, Deflate_LLCode *llcode, Deflate_Huffman *input_code, Buffer input, Deflate_Huffman *output_code) {
    Write *write = write_new(mem);
    write_bits(write, 1, 1);
    write_bits(write, 2, Deflate_BlockDynamic);
    deflate_huffman_dynamic_write(mem, output_code, write);
    deflate_lz_recode(mem, llcode, input_code, input, output_code, write);
    return write_get_written(write);
}

static Buffer deflate_write(Memory *mem, Buffer input) {
    Buffer result = {};
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

    // Encode using fixed huffman code and also collect frequency info
    Deflate_Huffman *fixed_code = deflate_huffman_fixed(mem);
    check(fixed_code);
    if (error) return buf_null();

    Deflate_Encode_Info frequency_info = {};
    Buffer fixed_output = deflate_write_fixed(mem, llcode, fixed_code, input, &frequency_info);
    if (error) return buf_null();
    if (enable_fixed) result = fixed_output;

    if (enable_dynamic) {
        // Re encode with the new huffman table
        Deflate_Huffman *dynamic_code = deflate_huffman_dynamic_create(mem, &frequency_info);
        check(dynamic_code);
        if (error) return buf_null();

        Buffer dynamic_output = deflate_write_dynamic(mem, llcode, fixed_code, fixed_output, dynamic_code);
        if (error) return buf_null();
        if (result.size == 0 || result.size > dynamic_output.size) result = dynamic_output;
    }

    if (enable_stored && (result.size == 0 || result.size > deflate_calculate_stored_block_size(input.size))) {
        result = deflate_write_stored(mem, input);
    }

    if (error) return buf_null();

    return result;
}

// Run a deflate/inflate testcase with a given input
static void deflate_test_buf(Memory *mem, Buffer input) {
    Buffer compressed = deflate_write(mem, input);
    Buffer decompressed = deflate_read(mem, compressed);
    check(buf_eq(decompressed, input));
}

static void deflate_test(Memory *mem) {
    deflate_test_buf(mem, str_buf("heeeeeeeeeeeeello hello"));
    deflate_test_buf(mem, str_buf("Hello World!"));
    deflate_test_buf(mem, str_buf("1234567"));
    deflate_test_buf(mem, str_buf(""));
    deflate_test_buf(mem, str_buf("0"));
    deflate_test_buf(mem, str_buf("0000000000000000"));
    deflate_test_buf(mem, str_buf("0000000000000001"));
    deflate_test_buf(mem, str_buf("1000000000000001"));

    Rand rng = {};
    for (u32 i = 0; i < 8; ++i) {
        size_t input_size = 1 << (i * 2);
        Buffer input = {mem_alloc_zero(mem, input_size), input_size};
        deflate_test_buf(mem, input);
        rand_bytes(&rng, input);
        deflate_test_buf(mem, input);
    }
}
