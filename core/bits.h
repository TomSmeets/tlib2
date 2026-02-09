#pragma once
#include "os.h"
#include "type.h"

typedef struct {
    size_t size;
    size_t index;
    u8 *data;
} Bits;

// Return number of remaining bits
static size_t bits_remaining(Bits *bits) {
    return bits->size - bits->index;
}

// Consume a number of bits (little endian)
static u32 bits_read(Bits *bits, u32 count) {
    assert(count <= 32);
    assert(bits->index + count <= bits->size);
    u32 out = 0;
    for (u32 i = 0; i < count; ++i) {
        size_t byte_ix = bits->index / 8;
        size_t bit_ix = bits->index % 8;
        out |= ((bits->data[byte_ix] >> bit_ix) & 1) << i;
        bits->index++;
    }
    return out;
}

// Discard bits until aligned
static void bits_byte_align(Bits *bits) {
    bits->index = (bits->index + 7) & ~7;
}

// ==== Testing ====
static void bits_test(void) {
    u32 data[] = {0b11000001111100001111000111001101};
    Bits bits = {
        .data = (u8 *)data,
        .size = sizeof(data) * 8,
    };
    assert(bits_read(&bits, 1) == 0b1);
    assert(bits_read(&bits, 1) == 0b0);
    assert(bits_read(&bits, 2) == 0b11);
    assert(bits_read(&bits, 2) == 0b00);
    assert(bits_read(&bits, 3) == 0b111);
    assert(bits_read(&bits, 3) == 0b000);
    assert(bits_read(&bits, 4) == 0b1111);
    assert(bits_read(&bits, 4) == 0b0000);
    assert(bits_read(&bits, 5) == 0b11111);
    assert(bits_read(&bits, 5) == 0b00000);
    assert(bits_read(&bits, 2) == 0b11);
    assert(bits.index == 32);

    bits.index = 0;
    bits_byte_align(&bits);
    assert(bits.index == 0);
    assert(bits_remaining(&bits) == 32);
    assert(bits_read(&bits, 7) == 0b1001101);
    assert(bits_read(&bits, 7) == 0b1100011);
    assert(bits.index == 14);
    assert(bits_remaining(&bits) == 32 - 14);
    bits_byte_align(&bits);
    assert(bits.index == 16);
    assert(bits_read(&bits, 16) == 0b1100000111110000);
    assert(bits.index == 32);
    assert(bits_remaining(&bits) == 0);
}
