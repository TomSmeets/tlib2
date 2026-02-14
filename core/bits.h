#pragma once
#include "os.h"
#include "type.h"

typedef struct {
    size_t size;
    size_t index;
    u8 *data;
} Bits;

static void bits_seek(Bits *bits, size_t offset) {
    bits->index = offset * 8;
}

// Reset read/write cursor to start
static void bits_restart(Bits *bits) {
    bits_seek(bits, 0);
}


// Create a new bit stream reader/writer
static Bits bits_from(size_t byte_count, void *data) {
    return (Bits){.size = byte_count * 8, .data = data};
}

// Return number of remaining bits
static size_t bits_remaining(Bits *bits) {
    return bits->size - bits->index;
}

// Consume a number of bits (little endian)
static u32 bits_read(Bits *bits, u32 count) {
    assert(count <= 32);
    assert(bits->index + count <= bits->size);

    u32 out = 0;
    if (bits->index % 8 == 0 && count % 8 == 0) {
        // Aligned
        for (u32 i = 0; i < count / 8; ++i) {
            out |= bits->data[bits->index / 8 + i] << (i*8);
        }
        bits->index += count;
    } else {
        for (u32 i = 0; i < count; ++i) {
            size_t byte_ix = bits->index / 8;
            size_t bit_ix = bits->index % 8;
            out |= ((bits->data[byte_ix] >> bit_ix) & 1) << i;
            bits->index++;
        }
    }
    return out;
}

// Write bits
static void bits_write(Bits *bits, u32 count, u32 data) {
    assert(count <= 32);
    assert(bits->index + count <= bits->size);
    for (u32 i = 0; i < count; ++i) {
        size_t byte_ix = bits->index / 8;
        size_t bit_ix = bits->index % 8;

        u8 mask = ~(1 << bit_ix);
        u8 value = ((data >> i) & 1) << bit_ix;
        bits->data[byte_ix] = (bits->data[byte_ix] & mask) | value;
        bits->index++;
    }
}

static void bits_write_bytes(Bits *bits, size_t count, void *data) {
    // Must be aligned
    assert(bits->index % 8 == 0);
    assert(bits->index + count * 8 <= bits->size);
    for (size_t i = 0; i < count; ++i) {
        size_t byte_ix = bits->index / 8;
        bits->data[byte_ix] = ((u8 *)data)[i];
        bits->index += 8;
    }
}

static void bits_read_bytes(Bits *bits, size_t count, void *data) {
    // Must be aligned
    assert(bits->index % 8 == 0);
    assert(bits->index + count * 8 <= bits->size);
    for (size_t i = 0; i < count; ++i) {
        size_t byte_ix = bits->index / 8;
        ((u8 *)data)[i] = bits->data[byte_ix];
        bits->index += 8;
    }
}

// Discard bits until aligned
static void bits_byte_align(Bits *bits) {
    bits->index = (bits->index + 7) & ~7;
}

// ==== Testing ====
static void bits_test(void) {
    u32 data[] = {0b11000001111100001111000111001101};
    Bits bits = bits_from(sizeof(data), data);
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

    bits_restart(&bits);

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

    bits_restart(&bits);
    bits_write(&bits, 4, 0b0000);
    bits_write(&bits, 4, 0b1111);
    bits_write(&bits, 1, 0b1);
    bits_write(&bits, 1, 0b0);

    // Overwrite
    bits.index = 3;
    bits_write(&bits, 2, 0b01);

    bits_restart(&bits);
    assert(bits_read(&bits, 4) == 0b1000);
    assert(bits_read(&bits, 4) == 0b1110);
    assert(bits_read(&bits, 1) == 0b1);
    assert(bits_read(&bits, 1) == 0b0);
}
