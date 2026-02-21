#pragma once
#include "huffman.h"
#include "mem.h"
#include "os.h"
#include "stream.h"
#include "type.h"

#if 0
typedef struct {
    size_t used;
    size_t capacity;
    u8 *buffer;
} Inflate;

static void inflate_byte(Inflate *inf, u8 data) {
    assert(inf->used < inf->capacity);
    inf->buffer[inf->used++] = data;
}

static void inflate_block(Inflate *inf, Bits *bits) {
    u32 b_final = bits_read(bits, 1);
    Deflate_Block b_type = bits_read(bits, 2);

    if (b_type == Deflate_BlockStored) {
        bits_byte_align(bits);
        u32 len = bits_read(bits, 16);
        u32 len_inv = bits_read(bits, 16);
        assert(len == (~len_inv & 0xffff));
        assert(bits->ix % 8 == 0);
        for (u32 i = 0; i < len; ++i) {
            inflate_byte(inf, bits_read(bits, 8));
        }
    }
}

static void deflate_write_block(Bits *bits, Deflate_Block0 *block) {
    bits_write(bits, 1, block->is_last);
    bits_write(bits, 2, block->type);

    if (block->type == Deflate_BlockStored) {
        bits_byte_align(bits);
        bits_write(bits, 16, block->size);
        bits_write(bits, 16, ~block->size);
        bits_write_bytes(bits, block->size, block->data);
    } else if (block->type == Deflate_BlockFixed) {
    } else if (block->type == Deflate_BlockDynamic) {
    }
}

// Huffman coding

static void deflate_test(void) {
    // https://www.youtube.com/watch?v=SJPvNi4HrWQ
    u8 buffer[1024];
    Bits bits = bits_from(sizeof(buffer), buffer);

    // Block Type 0
    bits_write(&bits, 1, 1);
    bits_write(&bits, 2, 0);
    bits_byte_align(&bits);
    char data[] = "Hello World!";
    u16 len = sizeof(data) - 1;
    bits_write(&bits, 1, len);
    bits_write(&bits, 1, ~len);
    bits_restart(&bits);
}
#endif

typedef enum {
    Deflate_BlockStored = 0,  // Raw data
    Deflate_BlockFixed = 1,   // Huffman
    Deflate_BlockDynamic = 2, // Huffman
} Deflate_BlockType;

typedef struct {
    bool is_last;
    Deflate_BlockType type;
} Deflate_Block;

static void deflate_read_block(Memory *mem, Stream *input, Stream *output) {
}


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

typedef struct {
    u8 length_bits[29];
    u8 distance_bits[30];
    u32 length_offset[29];
    u32 distance_offset[30];
} Deflate_LLCode;

static Deflate_LLCode * deflate_new_llcode(Memory *mem) {
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
    code->length_offset[0]  = 3;
    code->length_offset[28] = 258;
    for (u32 i = 1; i < 28; ++i) {
        u32 start = code->length_offset[i - 1];
        u32 bits  = code->length_bits[i - 1];
        code->length_offset[i] = start + (1 << bits);
    }

    // Distance offset
    code->distance_offset[0]  = 1;
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

    // 1. LL Code Literal/Length code
    // 2. Distance code -> encoded distance

    // Data is encoded as a stream of symbols encoded by a prefix code
    //   0-255 -> regular byte
    //     256 -> End of block
    // 257-285 -> Back references

    Stream *output = stream_new(mem);
    while (1) {
        bool is_last = stream_read_bits(input, 1);
        Deflate_BlockType type = stream_read_bits(input, 2);
        if (type == Deflate_BlockStored) {
            u16 size = stream_read_u16(input);
            u16 size_check = stream_read_u16(input);
            assert(size == ((~size_check) & 0xffff));

            for (size_t i = 0; i < size; ++i) {
                stream_write_u8(output, stream_read_u8(input));
            }
        } else if (type == Deflate_BlockFixed) {
            Deflate_LLCode *code = deflate_new_llcode(mem);
            Deflate_Huffman *huff = deflate_create_fixed_huffman(mem);

            while (1) {
                u32 symbol = huffman_read(huff->length, input);

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

                    u32 distance_code = huffman_read(huff->distance, input);
                    u32 distance = deflate_read_distance(code, input, distance_code);

                    size_t cursor = output->cursor - distance;
                    for (size_t i = 0; i < length; ++i) {
                        u8 c = output->buffer[cursor + i];
                        stream_write_u8(output, c);
                    }
                }
            }
        } else if (type == Deflate_BlockDynamic) {
            // TODO
            assert(false);
        } else {
            assert(false);
        }
        if (is_last) break;
    }
    return (Buffer){output->buffer, output->size};
}
