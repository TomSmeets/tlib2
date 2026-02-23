// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// stream.h - A dynamic Buffer for incremental reading and writing
#pragma once
#include "error.h"
#include "mem.h"
#include "type.h"

// A generic data stream
// - Can contain raw memory data
// - Can be backed by a File
typedef struct {
    u8 *buffer;

    // Backing buffer size
    size_t capacity;

    // Size of valid data
    size_t size;

    // Read/Write cursor
    size_t cursor;

    size_t bit_ix;

    // Optional memory for buffer reallocation
    Memory *mem;
} Stream;

// Create a stream backed by memory
static Stream stream_from(Buffer buf) {
    return (Stream){
        .buffer = buf.data,
        .capacity = buf.size,
        .size = buf.size,
    };
}

// Create a writable stream backed by memory
static Stream *stream_new(Memory *mem) {
    Stream *stream = mem_struct(mem, Stream);
    stream->mem = mem;
    return stream;
}

static Buffer stream_to_buffer(Stream *stream) {
    return (Buffer){stream->buffer, stream->size};
}

// Return current location
static size_t stream_cursor(Stream *stream) {
    return stream->cursor;
}

// Seek to a location
static void stream_seek(Stream *stream, size_t index) {
    // Cursor can be anywhere, even outside the current file
    stream->cursor = index;
    stream->bit_ix = 0;
}

// Clear data and go to start
static void stream_clear(Stream *stream) {
    stream->cursor = 0;
    stream->bit_ix = 0;
    stream->size = 0;
}

// Return if the stream has reached the end
static bool stream_eof(Stream *stream) {
    return stream->cursor >= stream->size;
}

// Grow backing buffer so that it fits at least 'needed' data
static bool stream_reserve(Stream *stream, size_t reserve) {
    // This must always be true
    try(stream->size <= stream->capacity);

    // Computed needed capacity
    size_t capacity_needed = stream->cursor + reserve;

    // Check if we dont need to grow
    if (capacity_needed <= stream->capacity) return true;

    // Check if reallocating is possible
    try(stream->mem);

    // Compute new capacity, growing in powers of two
    size_t new_capacity = MAX(stream->capacity, 16);
    while (stream->cursor + reserve > new_capacity) new_capacity *= 2;

    // Allocate new data and copy existing data to the new buffer
    u8 *new_data = mem_array(stream->mem, u8, new_capacity);
    mem_copy(new_data, stream->buffer, stream->size);
    stream->capacity = new_capacity;
    stream->buffer = new_data;
    return true;
}

// Write an aligned byte from the stream
static void stream_write_u8(Stream *stream, u8 data) {
    stream->bit_ix = 0;
    if (!stream_reserve(stream, 1)) return;
    stream->buffer[stream->cursor++] = data;
    if (stream->size < stream->cursor) stream->size = stream->cursor;
}

// Read an aligned byte from the stream
static u8 stream_read_u8(Stream *stream) {
    stream->bit_ix = 0;
    if (stream_eof(stream)) return 0;
    return stream->buffer[stream->cursor++];
}

// Write a single bit from the stream
static void stream_write_bit(Stream *stream, bool bit) {
    // New bit
    if (stream->bit_ix == 0) stream_write_u8(stream, 0);
    assert(stream->cursor > 0);

    u8 *byte = &stream->buffer[stream->cursor - 1];

    // Remove original bit
    *byte &= ~(1 << stream->bit_ix);

    // Write bit
    *byte |= bit << stream->bit_ix;

    // Advance to next bit
    stream->bit_ix++;
    stream->bit_ix %= 8;
}

// Read a single bit from the stream
static bool stream_read_bit(Stream *stream) {
    // New bit
    if (stream->bit_ix == 0) stream_read_u8(stream);
    assert(stream->cursor > 0);

    u8 byte = stream->buffer[stream->cursor - 1];
    u8 bit = (byte >> stream->bit_ix) & 1;

    // Advance to next bit
    stream->bit_ix++;
    stream->bit_ix %= 8;
    return bit;
}

// Read a number of bits
static u32 stream_read_bits(Stream *stream, u32 count) {
    u32 out = 0;
    for (u32 i = 0; i < count; ++i) {
        out |= (u32)stream_read_bit(stream) << i;
    }
    return out;
}

// Read a number of bits
static u32 stream_read_bits_be(Stream *stream, u32 count) {
    u32 out = 0;
    for (u32 i = 0; i < count; ++i) {
        out |= (u32)stream_read_bit(stream) << (count - i - 1);
    }
    return out;
}

// Write a number of bits
static void stream_write_bits(Stream *stream, u32 count, u32 bits) {
    for (u32 i = 0; i < count; ++i) {
        stream_write_bit(stream, (bits >> i) & 1);
    }
}

// Write a number of bits (big endian)
static void stream_write_bits_be(Stream *stream, u32 count, u32 bits) {
    for (u32 i = 0; i < count; ++i) {
        stream_write_bit(stream, (bits >> (count - i - 1)) & 1);
    }
}

#if 0
// Align up to a byte
static void stream_align(Stream *stream) {
    assert(stream->bit_ix < 8);
    if (stream->bit_ix) {
        stream->bit_ix = 0;
        stream->size++;
    }
}



// Read a single bit from the stream
static bool stream_read_bit(Stream *stream) {
    if (stream_eof(stream)) return 0;
    u8 byte = stream->buffer[stream->cursor];
    u8 bit = (byte >> stream->bit_ix) & 1;
    stream_next_bit(stream);
    return bit;
}

#endif

// // Read a number of little endian bytes
// static u32 stream_read_bytes(Stream *stream, u32 count) {
//     assert(count <= 4);
//     u32 res = 0;
//     for (u32 i = 0; i < count; ++i) {
//         res |= stream_read_u8(stream) << (i * 8);
//     }
//     return res;
// }

// static void stream_write_bytes(Stream *stream, u32 count, u32 data) {
//     assert(count <= 4);
//     for (u32 i = 0; i < count; ++i) {
//         stream_write_u8(stream, (data >> (i * 8)) & 0xff);
//     }
// }

static u16 stream_read_u16(Stream *stream) {
    u16 out = 0;
    out |= (u16)stream_read_u8(stream) << (0 * 8);
    out |= (u16)stream_read_u8(stream) << (1 * 8);
    return out;
}

static void stream_write_u16(Stream *stream, u16 data) {
    stream_write_u8(stream, (data >> (0 * 8)) & 0xff);
    stream_write_u8(stream, (data >> (1 * 8)) & 0xff);
}

static u32 stream_read_u32(Stream *stream) {
    u32 out = 0;
    out |= (u32)stream_read_u8(stream) << (0 * 8);
    out |= (u32)stream_read_u8(stream) << (1 * 8);
    out |= (u32)stream_read_u8(stream) << (2 * 8);
    out |= (u32)stream_read_u8(stream) << (3 * 8);
    return out;
}

static void stream_write_u32(Stream *stream, u32 data) {
    stream_write_u8(stream, (data >> (0 * 8)) & 0xff);
    stream_write_u8(stream, (data >> (1 * 8)) & 0xff);
    stream_write_u8(stream, (data >> (2 * 8)) & 0xff);
    stream_write_u8(stream, (data >> (3 * 8)) & 0xff);
}

static bool stream_read_bytes(Stream *stream, size_t count, u8 *data) {
    if (stream->cursor + count > stream->size) return false;
    for (size_t i = 0; i < count; ++i) {
        data[i] = stream_read_u8(stream);
    }
    return true;
}

static bool stream_write_bytes(Stream *stream, size_t count, u8 *data) {
    if (!stream_reserve(stream, count)) return false;
    for (size_t i = 0; i < count; ++i) {
        stream_write_u8(stream, data[i]);
    }
    return true;
}

static bool stream_write_buffer(Stream *stream, Buffer buffer) {
    return stream_write_bytes(stream, buffer.size, buffer.data);
}

static bool stream_read_buffer(Stream *stream, Buffer buffer) {
    return stream_read_bytes(stream, buffer.size, buffer.data);
}

static bool stream_from_file(Stream *stream, File *input) {
    u8 buffer[1024];
    for (;;) {
        size_t requested = sizeof(buffer);
        size_t used = 0;
        try(os_read(input, buffer, requested, &used));
        stream_write_bytes(stream, used, buffer);
        if (used < requested) break;
    }
    return 1;
}

static bool stream_to_file(Stream *stream, File *output) {
    return os_write(output, stream->buffer, stream->size, 0);
}

// Testing
static bool stream_test(void) {
    Memory *mem = mem_new();

    // Basic test
    Stream *stream = stream_new(mem);
    stream_write_u8(stream, 1);
    stream_write_u8(stream, 2);
    stream_write_u8(stream, 3);
    try(stream->size == 3);
    try(stream->cursor == stream->size);
    try(stream->size <= stream->capacity);
    try(stream->buffer[0] == 1);
    try(stream->buffer[1] == 2);
    try(stream->buffer[2] == 3);

    // Reading back data
    stream_seek(stream, 0);
    try(stream_read_u8(stream) == 1);
    try(stream_read_u8(stream) == 2);
    try(stream_read_u8(stream) == 3);
    try(stream_eof(stream));

    size_t cur = stream_cursor(stream);
    try(cur == 3);

    stream_write_u32(stream, 0x12345678);
    try(stream_cursor(stream) == 7);

    stream_seek(stream, cur);
    try(stream_read_u8(stream) == 0x78);
    try(stream_read_u8(stream) == 0x56);
    try(stream_read_u8(stream) == 0x34);
    try(stream_read_u8(stream) == 0x12);
    try(stream_eof(stream));

    stream_clear(stream);
    try(stream_eof(stream));
    try(stream->cursor == 0);
    try(stream->bit_ix == 0);

    stream_write_bit(stream, 1);
    try(stream->cursor == 1);
    try(stream->bit_ix == 1);

    stream_write_bit(stream, 0);
    stream_write_bit(stream, 1);
    stream_write_bit(stream, 1);
    stream_write_bit(stream, 0);
    stream_write_bit(stream, 0);
    stream_write_bit(stream, 0);
    stream_write_bit(stream, 1);
    try(stream->cursor == 1);
    try(stream->bit_ix == 0);

    // Extra
    stream_write_bit(stream, 1);
    stream_write_bit(stream, 1);

    stream_seek(stream, 0);
    try(stream_read_u8(stream) == 0b10001101);
    try(stream_read_u8(stream) == 0b00000011);
    try(stream_eof(stream));

    stream_seek(stream, 0);
    try(stream_read_bit(stream) == 1);
    try(stream_read_bit(stream) == 0);
    try(stream_read_bit(stream) == 1);
    try(stream_read_bit(stream) == 1);
    try(stream_read_bit(stream) == 0);
    try(stream_read_bit(stream) == 0);
    try(stream_read_bit(stream) == 0);
    try(stream_read_bit(stream) == 1);
    try(stream_read_bit(stream) == 1);
    try(stream_read_bit(stream) == 1);
    try(stream_eof(stream));
    stream_seek(stream, 0);

    try(stream_read_bits(stream, 10) == 0b1110001101);
    try(stream_eof(stream));

    // Write bits
    stream_clear(stream);
    stream_write_bits(stream, 7, 0b11111111);
    stream_write_bits(stream, 7, 0b10000000);
    stream_write_bits(stream, 7, 0b11111111);
    stream_write_bits(stream, 7, 0b10101010);
    stream_seek(stream, 0);
    try(stream_read_bits(stream, 7) == 0b01111111);
    try(stream_read_bits(stream, 7) == 0b00000000);
    try(stream_read_bits(stream, 7) == 0b01111111);
    try(stream_read_bits(stream, 7) == 0b00101010);
    try(stream_eof(stream));
    try(stream->cursor == 4);
    try(stream->size == 4);
    try(stream->bit_ix == 4);

    // Another test
    stream_clear(stream);

    u32 data[] = {0b11000001111100001111000111001101};
    Stream bits = stream_from(buf_from(data, sizeof(data)));
    try(stream_read_bits(&bits, 1) == 0b1);
    try(stream_read_bits(&bits, 1) == 0b0);
    try(stream_read_bits(&bits, 2) == 0b11);
    try(stream_read_bits(&bits, 2) == 0b00);
    try(stream_read_bits(&bits, 3) == 0b111);
    try(stream_read_bits(&bits, 3) == 0b000);
    try(stream_read_bits(&bits, 4) == 0b1111);
    try(stream_read_bits(&bits, 4) == 0b0000);
    try(stream_read_bits(&bits, 5) == 0b11111);
    try(stream_read_bits(&bits, 5) == 0b00000);
    try(stream_read_bits(&bits, 2) == 0b11);
    try(bits.bit_ix == 0);
    try(bits.cursor == 4);

    stream_seek(&bits, 0);
    try(stream_read_bits(&bits, 7) == 0b1001101);
    try(stream_read_bits(&bits, 7) == 0b1100011);
    try(bits.bit_ix == 6);

    // Should skip the 2 bits
    try(stream_read_u8(&bits) == 0b11110000);
    try(bits.bit_ix == 0);

    try(stream_read_bits(&bits, 1) == 1);
    try(stream_read_bits(&bits, 1) == 0);

    stream_seek(&bits, 0);
    try(stream_read_bits(&bits, 32) == 0b11000001111100001111000111001101);
    try(stream_eof(&bits));

    stream_seek(&bits, 0);
    try(stream_read_u32(&bits) == 0b11000001111100001111000111001101);
    try(stream_eof(&bits));

    stream_clear(&bits);
    stream_write_bits(&bits, 4, 0b0000);
    stream_write_bits(&bits, 4, 0b1111);
    stream_write_bits(&bits, 1, 0b1);
    stream_write_bits(&bits, 1, 0b0);

    stream_seek(&bits, 0);
    try(stream_read_bits(&bits, 4) == 0b0000);
    try(stream_read_bits(&bits, 4) == 0b1111);
    try(stream_read_bits(&bits, 1) == 0b1);
    try(stream_read_bits(&bits, 1) == 0b0);
    return 1;
}

// NOTE: make fmt also use stream?
// static void fmt_int(Stream *fmt, u64 value) {
//     fmt_s(fmt, "Hello");
//     fmt_s(fmt, "World");
//     fmt_eol(fmt);
// }
