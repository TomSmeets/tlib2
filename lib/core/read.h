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

static u16 read_u16(Read *read) {
    u16 out = 0;
    out |= (u16)read_u8(read) << (0 * 8);
    out |= (u16)read_u8(read) << (1 * 8);
    return out;
}

static u32 read_u24(Read *read) {
    u32 data = 0;
    data |= (u32)read_u8(read) << (0 * 8);
    data |= (u32)read_u8(read) << (1 * 8);
    data |= (u32)read_u8(read) << (2 * 8);
    return data;
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

// Parse unsigned LEB128 integer
static u64 read_leb128_ex(Read *read, bool is_signed) {
    u64 value = 0;
    u32 shift = 0;
    for (;;) {
        u8 byte = read_u8(read);
        u8 byte_low = byte & 0x7f;
        u8 byte_high = byte & 0x80;
        value |= (u64)byte_low << shift;
        if (byte_high == 0) break;
        shift += 7;
    }

    if (is_signed) {
        u32 move = 64 - shift;

        // Sign extend
        i64 s_value = value << move;

        // Shift back while sign extending
        s_value >>= move;

        value = s_value;
    }
    return value;
}

// read unsigned LEB128 integer
static u64 read_uleb128(Read *read) {
    return read_leb128_ex(read, 0);
}

// read signed LEB128 integer
static i64 read_ileb128(Read *read) {
    return read_leb128_ex(read, 1);
}

// Read a single line or until end of buffer
static Buffer read_line(Read *read) {
    u64 start = read_cursor(read);
    while (1) {
        if (read_eof(read)) break;
        u8 chr = read_u8(read);
        if (chr == '\n') break;
    }
    return buf_slice(read->buffer, start, read->bytes_read - start);
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

    read = read_from(BUFFER(u8, 0b10010101, 0b10011010, 0b11101111, 0b0111010, ));
    check(read_uleb128(&read) == 123456789);
    check(read_eof(&read));

    read = read_from(str_buf("Hello\n\nWorld"));
    check(buf_eq(read_line(&read), str_buf("Hello\n")));
    check(buf_eq(read_line(&read), str_buf("\n")));
    check(buf_eq(read_line(&read), str_buf("World")));
    check(read_eof(&read));
}
