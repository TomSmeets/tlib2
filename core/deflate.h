// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// deflate.h - DEFLATE decompressor implementation
#pragma once
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

// ======== Huffman Table Creation ========

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

// Read the dynamic huffman table for block type 2 from the input stream
static Deflate_Huffman *deflate_huffman_dynamic_read(Memory *mem, Stream *input) {
    u32 length_count = stream_read_bits(input, 5) + 257;
    assert(length_count <= 286);

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
    u8 lengths[286 + 30];
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

// ======== LL Code ========

// Symbol to actual length / distance encoding
// Each symbol starts at some offset is followed
// by a number of bits
typedef struct {
    // To calculate the length value:
    // - length_value = length_offset[sym] + read(length_bits[sym])
    u8 length_bits[29];
    u32 length_offset[29];

    // To calculate the distance value:
    // - distance_value = distance_offset[sym] + read(distance_bits[sym])
    u8 distance_bits[30];
    u32 distance_offset[30];
} Deflate_LLCode;

static void ll_encode_length(Deflate_LLCode *code, u32 length, Stream *output, u32 *length_symbol) {
    for (u32 i = 0; i < array_count(code->length_offset); ++i) {
    }
}

// Create the fixed llcode table
static Deflate_LLCode *deflate_llcode_new(Memory *mem) {
    Deflate_LLCode *code = mem_struct(mem, Deflate_LLCode);

    // Length values
    for (u32 i = 4; i < 28; ++i) {
        code->length_bits[i] = (i - 4) / 4;
    }

    // Distance values
    for (u32 i = 2; i < 30; ++i) {
        code->distance_bits[i] = (i - 2) / 2;
    }

    // Length offset
    code->length_offset[0] = 3;
    code->length_offset[28] = 258;
    for (u32 i = 1; i < 28; ++i) {
        u32 start = code->length_offset[i - 1];
        u32 bits = code->length_bits[i - 1];
        code->length_offset[i] = start + (1 << bits);
    }

    // Distance offset
    code->distance_offset[0] = 1;
    for (u32 i = 1; i < 30; ++i) {
        u32 start = code->distance_offset[i - 1];
        u32 bits = code->distance_bits[i - 1];
        code->distance_offset[i] = start + (1 << bits);
    }
    return code;
}

// Read construct an absolute length by reading the extra bits from the input stream based on the length_symbol
static u32 deflate_llcode_length_read(Deflate_LLCode *code, Stream *input, u16 length_symbol) {
    assert(length_symbol < 29);
    u32 bits = code->length_bits[length_symbol];
    u32 offset = code->length_offset[length_symbol];
    u32 data = stream_read_bits(input, bits);
    return offset + data;
}

// Read construct an absolute distance by reading the extra bits from the input stream based on the distance_symbol
static u32 deflate_llcode_distance_read(Deflate_LLCode *code, Stream *input, u16 distance_symbol) {
    assert(distance_symbol < 30);
    u32 bits = code->distance_bits[distance_symbol];
    u32 offset = code->distance_offset[distance_symbol];
    u32 data = stream_read_bits(input, bits);
    return offset + data;
}

// ===== LZSS ====

// Compress the input data using LZSS and encode it with the provided huffman code
// The symbol frequencies in the optional argument 'info' are incremented if present
static bool deflate_lzss_encode(Memory *mem, Deflate_Huffman *code, Buffer input, Buffer *output, Deflate_Encode_Info *info) {
    Stream *output_stream = stream_new(mem);

    // Add values
    for (size_t i = 0; i < input.size; ++i) {
        // Decide what symbol to encode
        u16 symbol = input.data[i];

        // Write length symbol
        huffman_code_write(code->length, output_stream, symbol);
        if (info) info->length_freq[symbol]++;

        // TODO: smarter lzss compression
    }

    // End of block marker
    huffman_code_write(code->length, output_stream, 256);
    if (info) info->length_freq[256]++;

    *output = stream_to_buffer(output_stream);
    return ok();
}

// ======== Deflate implementation ========
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
