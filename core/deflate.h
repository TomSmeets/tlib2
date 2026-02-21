#pragma once
#include "huffman.h"
#include "mem.h"
#include "os.h"
#include "stream.h"
#include "type.h"

typedef enum {
    Deflate_BlockStored = 0,  // Raw uncompressed data
    Deflate_BlockFixed = 1,   // Fixed Huffman table + LZ77
    Deflate_BlockDynamic = 2, // Dynamic Huffman table + LZ77
} Deflate_BlockType;

typedef struct {
    Huffman *length;
    Huffman *distance;
} Deflate_Huffman;

static Deflate_Huffman *deflate_create_fixed_huffman(Memory *mem) {
    Deflate_Huffman *tab = mem_struct(mem, Deflate_Huffman);

    // length prefix code bit sizes
    u8 h_len_bits[288];
    for (u32 i = 0; i < 144; ++i) h_len_bits[i] = 8;
    for (u32 i = 144; i < 256; ++i) h_len_bits[i] = 9;
    for (u32 i = 256; i < 280; ++i) h_len_bits[i] = 7;
    for (u32 i = 280; i < 288; ++i) h_len_bits[i] = 8;
    Huffman *h_len = huffman_new(mem, array_count(h_len_bits), h_len_bits);

    // distance prefix code bit sizes are always 5
    u8 h_dist_bits[32];
    for (u32 i = 0; i < 32; ++i) h_dist_bits[i] = 5;
    Huffman *h_dist = huffman_new(mem, array_count(h_dist_bits), h_dist_bits);

    tab->length = h_len;
    tab->distance = h_dist;
    return tab;
}

static Deflate_Huffman *deflate_create_dynamic_huffman(Memory *mem, Stream *input) {
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

    Huffman *code_tree = huffman_new(mem, 19, code_lengths);

    u32 count = 0;
    u8 lengths[286 + 30];
    while (count < length_count + distance_count) {
        u32 symbol = huffman_read(code_tree, input);
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
    huffman->length = huffman_new(mem, length_count, lengths);
    huffman->distance = huffman_new(mem, distance_count, lengths + length_count);
    return huffman;
}

typedef struct {
    u8 length_bits[29];
    u8 distance_bits[30];
    u32 length_offset[29];
    u32 distance_offset[30];
} Deflate_LLCode;

static Deflate_LLCode *deflate_new_llcode(Memory *mem) {
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

static u32 deflate_read_length(Deflate_LLCode *code, Stream *input, u16 length_code) {
    assert(length_code < 29);
    u32 bits = code->length_bits[length_code];
    u32 offset = code->length_offset[length_code];
    u32 data = stream_read_bits(input, bits);
    return offset + data;
}

static u32 deflate_read_distance(Deflate_LLCode *code, Stream *input, u16 distance_code) {
    assert(distance_code < 30);
    u32 bits = code->distance_bits[distance_code];
    u32 offset = code->distance_offset[distance_code];
    u32 data = stream_read_bits(input, bits);
    return offset + data;
}

static Buffer deflate_read(Memory *mem, Stream *input) {
    Stream *output = stream_new(mem);
    for (;;) {
        bool is_last = stream_read_bits(input, 1);
        Deflate_BlockType type = stream_read_bits(input, 2);

        // Stored bock
        if (type == Deflate_BlockStored) {
            u16 size = stream_read_u16(input);
            u16 size_check = stream_read_u16(input);
            assert(size == ((~size_check) & 0xffff));

            for (size_t i = 0; i < size; ++i) {
                stream_write_u8(output, stream_read_u8(input));
            }
        } else {
            Deflate_LLCode *code = deflate_new_llcode(mem);
            Deflate_Huffman *tree = 0;
            if (type == Deflate_BlockFixed) tree = deflate_create_fixed_huffman(mem);
            if (type == Deflate_BlockDynamic) tree = deflate_create_dynamic_huffman(mem, input);

            while (1) {
                u32 symbol = huffman_read(tree->length, input);

                // Symbol must be valid
                assert(symbol < 288);

                // End of block
                if (symbol == 256) break;

                // Normal byte
                if (symbol < 256) stream_write_u8(output, symbol);

                // LZ77 sequence
                if (symbol > 256) {
                    u32 length_code = symbol - 257;
                    u32 length = deflate_read_length(code, input, length_code);

                    u32 distance_code = huffman_read(tree->distance, input);
                    u32 distance = deflate_read_distance(code, input, distance_code);

                    size_t cursor = output->cursor - distance;
                    for (size_t i = 0; i < length; ++i) {
                        u8 c = output->buffer[cursor + i];
                        stream_write_u8(output, c);
                    }
                }
            }
        }

        if (is_last) break;
    }
    return (Buffer){output->buffer, output->size};
}
