#pragma once
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

    // Cursor bit offset
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

// Return current location
static size_t stream_cursor(Stream *stream) {
    return stream->cursor;
}

// Seek to a location
static void stream_seek(Stream *stream, size_t index) {
    // Cursor can be anywhere, even outside the current file
    stream->cursor = index;
}

// Return if the stream has reached the end
static bool stream_eof(Stream *stream) {
    return stream->cursor >= stream->size;
}

// Grow backing buffer so that it fits at least 'needed' data
static bool stream_reserve(Stream *stream, size_t reserve) {
    // This must always be true
    assert(stream->size <= stream->capacity);

    // Computed needed capacity
    size_t capacity_needed = stream->cursor + reserve;

    // Check if we dont need to grow
    if (capacity_needed <= stream->capacity) return true;

    // Check if reallocating is possible
    if (!stream->mem) return false;

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

// Align up to a byte
static void stream_align(Stream *stream) {
    assert(stream->bit_ix < 8);
    if (stream->bit_ix) {
        stream->bit_ix = 0;
        stream->size++;
    }
}

// Write an aligned byte from the stream
static void stream_write_u8(Stream *stream, u8 data) {
    if (!stream_reserve(stream, 1)) return;
    stream->buffer[stream->cursor++] = data;
    if (stream->size < stream->cursor) stream->size = stream->cursor;
}

// Read an aligned byte from the stream
static u8 stream_read_u8(Stream *stream) {
    if (stream_eof(stream)) return 0;
    return stream->buffer[stream->cursor++];
}

#if 0
static void stream_next_bit(Stream *stream) {
    stream->bit_ix++;
    if (stream->bit_ix == 8) {
        stream->bit_ix = 0;
        stream->cursor++;
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

// Read a single bit from the stream
static void stream_write_bit(Stream *stream, bool bit) {
    if (stream->size >= stream->capacity) return;
    u8 *byte = &stream->buffer[stream->size];

    // Remove original bit
    *byte &= ~(1 << stream->bit_ix);

    // Write bit
    *byte |= bit << stream->bit_ix;
    stream_next_bit(stream);
}

// Read a number of bits
static u32 stream_read_bits(Stream *stream, u32 count) {
    u32 out = 0;
    for (u32 i = 0; i < count; ++i) {
        out |= (u32)stream_read_bit(stream) << i;
    }
    return out;
}

// Write a number of bits
static void stream_write_bits(Stream *stream, u32 count, u32 bits) {
    for (u32 i = 0; i < count; ++i) {
        stream_write_bit(stream, (bits >> i) & 1);
    }
}

#endif

static u32 stream_read(Stream *stream, u32 count) {
    u32 res = 0;
    for (u32 i = 0; i < count; ++i) {
        res |= stream_read_u8(stream) << (i * 8);
    }
    return res;
}

static void stream_write(Stream *stream, u32 count, u32 data) {
    for (u32 i = 0; i < count; ++i) {
        stream_write_u8(stream, (data >> (i * 8)) & 0xff);
    }
}

// Testing
static void stream_test(void) {
    Memory *mem = mem_new();

    // Basic test
    Stream *stream = stream_new(mem);
    stream_write_u8(stream, 1);
    stream_write_u8(stream, 2);
    stream_write_u8(stream, 3);
    assert(stream->size == 3);
    assert(stream->cursor == stream->size);
    assert(stream->size <= stream->capacity);
    assert(stream->buffer[0] == 1);
    assert(stream->buffer[1] == 2);
    assert(stream->buffer[2] == 3);

    // Reading back data
    stream_seek(stream, 0);
    assert(stream_read_u8(stream) == 1);
    assert(stream_read_u8(stream) == 2);
    assert(stream_read_u8(stream) == 3);
    assert(stream_eof(stream));

    size_t cur = stream_cursor(stream);
    assert(cur == 3);

    stream_write(stream, 4, 0x12345678);
    assert(stream_cursor(stream) == 7);

    stream_seek(stream, cur);
    assert(stream_read_u8(stream) == 0x78);
    assert(stream_read_u8(stream) == 0x56);
    assert(stream_read_u8(stream) == 0x34);
    assert(stream_read_u8(stream) == 0x12);
    assert(stream_eof(stream));
}
