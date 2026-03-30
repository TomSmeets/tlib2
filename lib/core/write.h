// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// write.h - Dynamic Buffer writer
#pragma once
#include "buf.h"
#include "mem.h"

typedef struct {
    Buffer buffer;
    size_t bytes_written;

    // For writing bits
    u8 bit_ix;

    // Optional, allow reallocation when set
    Memory *mem;
} Write;

// Create a new writer that writes to the destination buffer
static Write write_from(Buffer buffer) {
    return (Write) { .buffer = buffer };
}

// Create a new dynamic writer that dynamically allocates memory
static Write *write_new(Memory *mem) {
    Write *write = mem_struct(mem, Write);
    write->mem = mem;
    return write;
}

// Return current written buffer
static Buffer write_get_written(Write *write) {
    return buf_take(write->buffer, write->bytes_written);
}

// Re-allocate buffer to the new size
static Buffer buf_realloc(Memory*mem, Buffer buf, size_t new_size) {
    buf.data = mem_realloc(mem, buf.data, buf.size, new_size);
    buf.size = new_size;
    return buf;
}

// Grow backing buffer so that it fits at least 'needed' data after the cursor
static void write_reserve(Write *write, size_t reserve) {
    assert(write->bytes_written <= write->buffer.size);

    // Check if we dont need to grow
    size_t capacity_needed = write->bytes_written + reserve;
    if (capacity_needed <= write->buffer.size) return;

    // Check if reallocating is possible
    check_or(write->mem) return;

    // Compute new capacity, growing in powers of two
    size_t new_capacity = MAX(write->buffer.size, 16);
    while (capacity_needed > new_capacity) new_capacity *= 2;

    // Allocate new data and copy existing data to the new buffer
    write->buffer = buf_realloc(write->mem, buf_take(write->buffer, write->bytes_written), new_capacity);
}

static void write_u8(Write *write, u8 value) {
    write_reserve(write, 1);
    check_or(write->bytes_written < write->buffer.size) return;
    write->bit_ix = 0;
    write->buffer.data[write->bytes_written++] = value;
}

// Get a reference to the latest written byte
static u8 *write_get_last_byte(Write *write) {
    // Write must be valid
    check_or(write->bytes_written > 0) return 0;
    return &write->buffer.data[write->bytes_written - 1];
}

// Write a single bit
static void write_bit(Write *write, u8 bit) {
    // Init accumulator
    if (write->bit_ix == 0) write_u8(write, 0);

    // Append bit
    u8 *byte = write_get_last_byte(write);
    if(byte) *byte |= (bit & 1) << write->bit_ix;

    // Advance to next bit
    write->bit_ix++;

    // Write finished byte
    if (write->bit_ix == 8) write->bit_ix = 0;
}

// Get current position
static size_t write_cursor(Write *write) {
    return write->bytes_written;
}

// Set current position
static void write_seek(Write *write, size_t cursor) {
    check_or(cursor <= write->buffer.size) cursor = write->buffer.size;
    write->bit_ix = 0;
    write->bytes_written = cursor;
}

// =================== Derived ===================

// Write two bytes as a little endian u16 from the stream
static void write_u16(Write *write, u16 data) {
    write_u8(write, (data >> (0 * 8)) & 0xff);
    write_u8(write, (data >> (1 * 8)) & 0xff);
}


// Write two bytes as a little endian u32 from the stream
static void write_u32(Write *write, u32 data) {
    write_u8(write, (data >> (0 * 8)) & 0xff);
    write_u8(write, (data >> (1 * 8)) & 0xff);
    write_u8(write, (data >> (2 * 8)) & 0xff);
    write_u8(write, (data >> (3 * 8)) & 0xff);
}

// Write the contents of the buffer to the stream
static void write_buffer(Write *write, Buffer buffer) {
    write_reserve(write, buffer.size);
    for (size_t i = 0; i < buffer.size; ++i) {
        write_u8(write, buffer.data[i]);
    }
}

// Write 'count' little-endian bits
static void write_bits(Write *write, u8 count, u32 bits) {
    check(count <= 32);
    for (u8 i = 0; i < count; ++i) {
        write_bit(write, (bits >> i) & 1);
    }
}

// Write 'count' big-endian bits
static void write_bits_be(Write *write, u8 count, u32 bits) {
    check(count <= 32);
    for (u8 i = 0; i < count; ++i) {
        write_bit(write, (bits >> (count - 1 - i)) & 1);
    }
}

// Repeat previously written data (As used in LZ77)
static void write_repeat(Write *write, size_t distance, size_t length) {
    // Already reserve space for the new data (not required, but a bit faster)
    if(length > distance) write_reserve(write, length - distance);
    
    // Check distance validity
    check_or(write->bytes_written >= distance) return;

    // Write data
    size_t start_index = write->bytes_written - distance;
    for (size_t offset = 0; offset < length; ++offset) {
        write_u8(write, write->buffer.data[start_index + offset]);
    }
}

// =========== Testing =======
static void test_write(void) {
    Memory *mem = mem_new();
    Write *write = write_new(mem);
    write_u8(write, 'H');
    write_u8(write, 'e');
    write_u8(write, 'l');
    write_u8(write, 'l');
    write_u8(write, 'o');
    mem_free(mem);
}
