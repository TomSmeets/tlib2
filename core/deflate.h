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

static Buffer deflate_read(Memory *mem, Stream *input) {
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
            // 1. LL Code Literal/Length code
            // 2. Distance code -> encoded distance

            // Data is encoded as a stream of symbols encoded by a prefix code
            //   0-255 -> regular byte
            //     256 -> End of block
            // 257-285 -> Back references

            // Symbols: 257 until 285
            u32 length_bits[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
            u32 length_start[29] = {3};
            for (u32 i = 1; i < array_count(length_bits) - 1; ++i) {
                length_start[i] = length_start[i - 1] + (1 << length_bits[i - 1]);
            }
            // note that this replaces the previous symbol '284' with length 31
            length_start[array_count(length_start) - 1] = 258;

            // Distance values
            u32 distance_bits[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
            u32 distance_start[30] = {1};
            for (u32 i = 1; i < array_count(distance_bits); ++i) {
                distance_start[i] = distance_start[i - 1] + (1 << distance_bits[i - 1]);
            }

            // length prefix code bit sizes
            Huffman *h_len = huffman_new(mem, 288);
            for (u32 i = 0; i < 144; ++i) huffman_add(h_len, 8);
            for (u32 i = 144; i < 256; ++i) huffman_add(h_len, 9);
            for (u32 i = 256; i < 280; ++i) huffman_add(h_len, 7);
            for (u32 i = 280; i < 288; ++i) huffman_add(h_len, 8);
            huffman_build(h_len);

            // distance prefix code bit sizes are always 5
            Huffman *h_dist = huffman_new(mem, 32);
            for (u32 i = 0; i < 32; ++i) huffman_add(h_dist, 5);
            huffman_build(h_dist);

            for (u32 s = 0; s < 288; ++s) {
                fmt_su(fout, "", s, ": ");
                fmt_u_ex(fout, h_len->code[s], 2, '0', h_len->len[s]);
                fmt_s(fout, "\n");
            }

            while (1) {
                Huffman_Result res = huffman_read(h_len, input);
                fmt_su(fout, "x: ", res.symbol, " ");
                if (res.symbol < 256) fmt_c(fout, res.symbol);
                fmt_s(fout, "\n");

                assert(res.valid);
                if (res.symbol == 256) break;
                if (res.symbol < 256) {
                    stream_write_u8(output, res.symbol);
                    continue;
                }

                u32 length_code = res.symbol - 257;
                u32 length = length_start[length_code] + stream_read_bits(input, length_bits[length_code]);
                fmt_su(fout, "length: ", length, "\n");

                res = huffman_read(h_dist, input);
                assert(res.valid);
                u32 distance_code = res.symbol;
                u32 distance = distance_start[distance_code] + stream_read_bits(input, distance_bits[distance_code]);
                fmt_su(fout, "Distance: ", distance, "\n");

                size_t start = output->cursor - distance;
                for (size_t i = 0; i < length; ++i) {
                    u8 c = output->buffer[start + i];
                    fmt_s(fout, "r: ");
                    if (res.symbol < 256) fmt_c(fout, c);
                    fmt_s(fout, "\n");
                    stream_write_u8(output, output->buffer[start + i]);
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
