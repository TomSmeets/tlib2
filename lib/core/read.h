// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// read.h - Reader
#pragma once
#include "buf.h"
#include "error.h"
#include "str.h"
#include "type.h"

typedef struct {
    // Buffer to read from
    Buffer buffer;

    // Bytes consumed
    size_t bytes_read;

    // Number of bits consumed from last byte (if > 0)
    u8 bit_ix;
} Read;

// Create a new reader that reads from a buffer
static Read read_from(Buffer buffer) {
    return (Read){.buffer = buffer};
}

// Return true if no more bytes are able to be read
static bool read_eof(Read *read) {
    return read->bytes_read >= read->buffer.size;
}

// Return true if no more bits are able to be read
static bool read_bit_eof(Read *read) {
    return read_eof(read) && read->bit_ix == 0;
}

// Read a single byte
static u8 read_u8(Read *read) {
    read->bit_ix = 0;
    check_or(read->bytes_read < read->buffer.size) return 0;
    return read->buffer.data[read->bytes_read++];
}

// Get a reference to the latest written byte
static u8 read_get_last_byte(Read *read) {
    // Read must be valid
    check_or(read->bytes_read > 0) return 0;
    return read->buffer.data[read->bytes_read - 1];
}

// Read a single bit
static u8 read_bit(Read *read) {
    // Consume next byte
    if (read->bit_ix == 0) read_u8(read);

    // Extract bit
    u8 byte = read_get_last_byte(read);
    u8 bit = (byte >> read->bit_ix) & 1;

    // Advance to next bits
    read->bit_ix++;

    // Reset bits_read if the byte was finished
    if (read->bit_ix == 8) read->bit_ix = 0;
    return bit;
}

// Get current position
static size_t read_cursor(Read *read) {
    return read->bytes_read;
}

// Set current position
static void read_seek(Read *read, size_t cursor) {
    check_or(cursor <= read->buffer.size) cursor = read->buffer.size;
    read->bit_ix = 0;
    read->bytes_read = cursor;
}

// =================== Derived ===================

static u32 read_u16(Read *read) {
    u32 out = 0;
    out |= (u32)read_u8(read) << (0 * 8);
    out |= (u32)read_u8(read) << (1 * 8);
    return out;
}

static u32 read_u32(Read *read) {
    u32 out = 0;
    out |= (u32)read_u8(read) << (0 * 8);
    out |= (u32)read_u8(read) << (1 * 8);
    out |= (u32)read_u8(read) << (2 * 8);
    out |= (u32)read_u8(read) << (3 * 8);
    return out;
}

// Read 'count' little-endian bits
static u32 read_bits(Read *read, u32 count) {
    check(count <= 32);
    u32 ret = 0;
    for (u32 i = 0; i < count; ++i) {
        ret |= (u32)read_bit(read) << i;
    }
    return ret;
}

// =========== Testing =======
static void test_read(void) {
    Read read = read_from(str_buf("Hello World!"));
    check(read_u8(&read) == 'H');
    check(read_u8(&read) == 'e');
    check(read_eof(&read) == 0);
    check(read_bits(&read, 8) == 'l');
    // 'l' = 0110 1100
    check(read_bits(&read, 3) == 0b100);
    check(read_bits(&read, 1) == 0b1);
    check(read_bits(&read, 2) == 0b10);
    check(read_bits(&read, 2) == 0b01);
    check(read_bits(&read, 24) == (((u32)'W' << 16) | ((u32)' ' << 8) | (u32)'o'));
    read_seek(&read, 0);
    check(read_u8(&read) == 'H');

    read = read_from(buf_null());
    check(read_eof(&read));
}
