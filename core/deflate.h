#pragma once
#include "type.h"
#include "os.h"
#include "mem.h"
#include "bits.h"

typedef struct {
    size_t used;
    size_t capacity;
    u8 *buffer;
} Inflate;

typedef enum {
    Deflate_BlockStored = 0,   // Raw data
    Deflate_BlockFixed = 1,    // Huffman
    Deflate_BlockDynamic = 2,  // Huffman
    Deflate_BlockReserved = 3, // Invalid
} Deflate_Block;

static void inflate_byte(Inflate *inf, u8 data) {
    assert(inf->used < inf->capacity);
    inf->buffer[inf->used++] = data;
}

static void inflate_block(Inflate *inf, Bits *bits) {
    u32 b_final = bits_read(bits, 1);
    Deflate_Block b_type = bits_read(bits, 2);

    if(b_type == Deflate_BlockStored) {
        bits_align_to_byte_boundary(bits);
        u32 len = bits_read(bits, 16);
        u32 len_inv = bits_read(bits, 16);
        assert(len == (~len_inv & 0xffff));
        assert(bits->ix % 8 == 0);
        for (u32 i = 0; i < len; ++i) {
            inflate_byte(inf, bits_read(bits, 8));
        }
    }
}


// LZ77
static LZ77 *lz_init(Memory *mem, size_t capacity) {
    LZ77 *lz = mem_struct(mem, LZ77);
    lz->capacity = capacity;
    lz->buffer = mem_array(mem, u8, capacity);
    return lz;
}

static void lz77_decode_repeat(LZ77 *lz, size_t offset, size_t len) {
    assert(lz->used + len <= lz->capacity);
    assert(offset < lz->capacity);
    u8 *rx = lz->buffer + lz->used - offset;
    for (size_t i = 0; i < len; ++i) {
        lz->buffer[lz->used++] = *rx++;
    }
}

static void lz77_decode_data(LZ77 *lz, size_t len, u8 *data) {
    assert(lz->used + len <= lz->capacity);
    for (size_t i = 0; i < len; ++i) {
        lz->buffer[lz->used++] = *data++;
    }
}

// Huffman coding
