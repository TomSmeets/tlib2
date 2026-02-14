#pragma once
#include "mem.h"
#include "type.h"

// A generic data stream
// - Can contain raw memory data
// - Can be backed by a File
typedef struct {
    u8 *data;
    size_t size;
    size_t used;
    size_t bit_ix;

    // Optional: Allow for reallocation of backing memory
    Memory *mem;
} Stream;

// Create a stream backed by memory
static Stream stream_from(Buffer buf) {
    return (Stream){
        .data = buf.data,
        .size = buf.size,
        .used = 0,
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
    return stream->used;
}

// Seek to a location
static void stream_seek(Stream *stream, size_t index) {
    stream->used = index;
}

// Return if the stream has reached the end
static bool stream_eof(Stream *stream) {
    return stream->used >= stream->size;
}

// Grow stream so that it fits at least 'needed' data
static bool stream_reserve(Stream *stream, size_t reserve) {
    // Check if we need to grow
    if (stream->used + reserve <= stream->size) return true;

    // Check if we can allocate memory
    if (!stream->mem) return false;

    // Compute new size
    size_t new_size = MAX(stream->size, 64);
    while (stream->used + reserve > new_size) new_size *= 2;

    // Allocate new data and copy existing data to the new buffer
    u8 *new_data = mem_array(stream->mem, u8, new_size);
    mem_copy(new_data, stream->data, stream->used);
    stream->size = new_size;
    stream->data = new_data;
    return false;
}

static void stream_align(Stream *stream) {
    if (stream->bit_ix) {
        assert(stream->bit_ix < 8);
        stream->bit_ix = 0;
        stream->used++;
    }
}

// Write an aligned byte from the stream
static void stream_write_u8(Stream *stream, u8 data) {
    if (!stream_reserve(stream, 1)) return;
    stream->data[stream->used++] = data;
}

// Read an aligned byte from the stream
static u8 stream_read_u8(Stream *stream) {
    if (stream_eof(stream)) return 0;
    return stream->data[stream->used++];
}

static void stream_next_bit(Stream *stream) {
    stream->bit_ix++;
    if (stream->bit_ix == 8) {
        stream->bit_ix = 0;
        stream->used++;
    }
}

// Read a single bit from the stream
static bool stream_read_bit(Stream *stream) {
    if (stream->used >= stream->size) return 0;
    u8 byte = stream->data[stream->used];
    u8 bit = (byte >> stream->bit_ix) & 1;
    stream_next_bit(stream);
    return bit;
}

// Read a single bit from the stream
static void stream_write_bit(Stream *stream, bool bit) {
    if (stream->used >= stream->size) return;
    u8 *byte = &stream->data[stream->used];

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


// Testing
static void stream_test(void) {
}
