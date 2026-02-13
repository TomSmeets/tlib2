#pragma once
#include "bits.h"
#include "mem.h"
#include "os.h"
#include "type.h"

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

typedef struct {
    bool is_last;
    Deflate_Block type;
    u16 size;
    void *data;
} Deflate_Block0;

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

static Deflate_Block0 *deflate_read_block(Bits *bits, Memory *mem) {
    Deflate_Block0 *block = mem_struct(mem, Deflate_Block0);
    block->is_last = bits_read(bits, 1);
    block->type = bits_read(bits, 2);
    assert(block->type < 3);

    if (block->type == Deflate_BlockStored) {
        bits_byte_align(bits);
        u16 size = bits_read(bits, 16);
        u16 size_check = bits_read(bits, 16);
        assert(size == ~size_check);

        u8 *data = mem_array(mem, u8, size);
        bits_read_bytes(bits, size, data);
        block->size = size;
        block->data = data;
    } else if (block->type == Deflate_BlockFixed) {
    } else if (block->type == Deflate_BlockDynamic) {
    }
    return block;
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
