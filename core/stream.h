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

static Buffer stream_buffer(Stream *stream) {
    return (Buffer) { stream->buffer, stream->size };
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

static void stream_read_bytes(Stream *stream, size_t count, u8 *data) {
    for (size_t i = 0; i < count; ++i) {
        data[i] = stream_read_u8(stream);
    }
}

static void stream_write_bytes(Stream *stream, size_t count, u8 *data) {
    for (size_t i = 0; i < count; ++i) {
        stream_write_u8(stream, data[i]);
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

    stream_write_u32(stream, 0x12345678);
    assert(stream_cursor(stream) == 7);

    stream_seek(stream, cur);
    assert(stream_read_u8(stream) == 0x78);
    assert(stream_read_u8(stream) == 0x56);
    assert(stream_read_u8(stream) == 0x34);
    assert(stream_read_u8(stream) == 0x12);
    assert(stream_eof(stream));

    stream_clear(stream);
    assert(stream_eof(stream));
    assert(stream->cursor == 0);
    assert(stream->bit_ix == 0);

    stream_write_bit(stream, 1);
    assert(stream->cursor == 1);
    assert(stream->bit_ix == 1);

    stream_write_bit(stream, 0);
    stream_write_bit(stream, 1);
    stream_write_bit(stream, 1);
    stream_write_bit(stream, 0);
    stream_write_bit(stream, 0);
    stream_write_bit(stream, 0);
    stream_write_bit(stream, 1);
    assert(stream->cursor == 1);
    assert(stream->bit_ix == 0);

    // Extra
    stream_write_bit(stream, 1);
    stream_write_bit(stream, 1);

    stream_seek(stream, 0);
    assert(stream_read_u8(stream) == 0b10001101);
    assert(stream_read_u8(stream) == 0b00000011);
    assert(stream_eof(stream));

    stream_seek(stream, 0);
    assert(stream_read_bit(stream) == 1);
    assert(stream_read_bit(stream) == 0);
    assert(stream_read_bit(stream) == 1);
    assert(stream_read_bit(stream) == 1);
    assert(stream_read_bit(stream) == 0);
    assert(stream_read_bit(stream) == 0);
    assert(stream_read_bit(stream) == 0);
    assert(stream_read_bit(stream) == 1);
    assert(stream_read_bit(stream) == 1);
    assert(stream_read_bit(stream) == 1);
    assert(stream_eof(stream));
    stream_seek(stream, 0);

    assert(stream_read_bits(stream, 10) == 0b1110001101);
    assert(stream_eof(stream));

    // Write bits
    stream_clear(stream);
    stream_write_bits(stream, 7, 0b11111111);
    stream_write_bits(stream, 7, 0b10000000);
    stream_write_bits(stream, 7, 0b11111111);
    stream_write_bits(stream, 7, 0b10101010);
    stream_seek(stream, 0);
    assert(stream_read_bits(stream, 7) == 0b01111111);
    assert(stream_read_bits(stream, 7) == 0b00000000);
    assert(stream_read_bits(stream, 7) == 0b01111111);
    assert(stream_read_bits(stream, 7) == 0b00101010);
    assert(stream_eof(stream));
    assert(stream->cursor == 4);
    assert(stream->size == 4);
    assert(stream->bit_ix == 4);

    // Another test
    stream_clear(stream);

    u32 data[] = {0b11000001111100001111000111001101};
    Stream bits = stream_from(buf_from(data, sizeof(data)));
    assert(stream_read_bits(&bits, 1) == 0b1);
    assert(stream_read_bits(&bits, 1) == 0b0);
    assert(stream_read_bits(&bits, 2) == 0b11);
    assert(stream_read_bits(&bits, 2) == 0b00);
    assert(stream_read_bits(&bits, 3) == 0b111);
    assert(stream_read_bits(&bits, 3) == 0b000);
    assert(stream_read_bits(&bits, 4) == 0b1111);
    assert(stream_read_bits(&bits, 4) == 0b0000);
    assert(stream_read_bits(&bits, 5) == 0b11111);
    assert(stream_read_bits(&bits, 5) == 0b00000);
    assert(stream_read_bits(&bits, 2) == 0b11);
    assert(bits.bit_ix == 0);
    assert(bits.cursor == 4);

    stream_seek(&bits, 0);
    assert(stream_read_bits(&bits, 7) == 0b1001101);
    assert(stream_read_bits(&bits, 7) == 0b1100011);
    assert(bits.bit_ix == 6);

    // Should skip the 2 bits
    assert(stream_read_u8(&bits) == 0b11110000);
    assert(bits.bit_ix == 0);

    assert(stream_read_bits(&bits, 1) == 1);
    assert(stream_read_bits(&bits, 1) == 0);

    stream_seek(&bits, 0);
    assert(stream_read_bits(&bits, 32) == 0b11000001111100001111000111001101);
    assert(stream_eof(&bits));

    stream_seek(&bits, 0);
    assert(stream_read_u32(&bits) == 0b11000001111100001111000111001101);
    assert(stream_eof(&bits));

    stream_clear(&bits);
    stream_write_bits(&bits, 4, 0b0000);
    stream_write_bits(&bits, 4, 0b1111);
    stream_write_bits(&bits, 1, 0b1);
    stream_write_bits(&bits, 1, 0b0);

    stream_seek(&bits, 0);
    assert(stream_read_bits(&bits, 4) == 0b0000);
    assert(stream_read_bits(&bits, 4) == 0b1111);
    assert(stream_read_bits(&bits, 1) == 0b1);
    assert(stream_read_bits(&bits, 1) == 0b0);
}

// NOTE: make fmt also use stream?
// static void fmt_int(Stream *fmt, u64 value) {
//     fmt_s(fmt, "Hello");
//     fmt_s(fmt, "World");
//     fmt_eol(fmt);
// }
