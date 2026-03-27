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

    // Total allocation size of the buffer
    size_t capacity;

    // Part of the buffer that was 'written' and is valid.
    size_t size;

    // Read/Write cursor, can move around freely
    //
    // The cursor is allowed to point outside of the written and allocate region
    // If outside the range, a 'Read' will return an error.
    size_t cursor;

    // Current bit index in the byte at the cursor
    size_t bit_ix;

    // Optional memory for buffer reallocation during writes
    Memory *mem;

    // NOTE: There is no backing file, this is left out for simplicity.
    // - On Linux/Windows -> you can just read the entire file or memory-map a big file
    // - On WASM          -> The files are already memory
} Stream;

// ========================= Stream Construction ==================================

// Create an empty stream backed by an allocator
static Stream *stream_new(Memory *mem) {
    Stream *stream = mem_struct(mem, Stream);
    stream->mem = mem;
    return stream;
}

// Create a fixed-size stream backed with the buffer contents
static Stream stream_from_buffer(Buffer buf) {
    return (Stream){
        .buffer = buf.data,
        .capacity = buf.size,
        .size = buf.size,
    };
}

// ======================= Stream Destruction ====================================

// View all written data as a buffer (ignoring cursor position)
static Buffer stream_as_buffer(Stream *stream) {
    return (Buffer){stream->buffer, stream->size};
}

// =========================== Seeking =========================================

// Return current cursor byte position
static size_t stream_cursor(Stream *stream) {
    return stream->cursor;
}

// Seek to byte offset
static void stream_seek(Stream *stream, size_t index) {
    // Cursor can be anywhere, even outside the current file
    stream->cursor = index;
    stream->bit_ix = 0;
}

// Seek to the start of the stream
static void stream_restart(Stream *stream) {
    stream_seek(stream, 0);
}

// ===================== Checks ============================

// Return if the stream has reached the end of the written data
static bool stream_eof(Stream *stream) {
    return stream->cursor >= stream->size;
}

// Number of written bytes that remain for reading after the current position
static size_t stream_remaining(Stream *stream) {
    return stream->size - stream->cursor;
}

static size_t stream_size(Stream *stream) {
    return stream->size;
}
// ===================== Clear / Reserve ============================

// Reset entire stream, discard all written data
static void stream_clear(Stream *stream) {
    stream->cursor = 0;
    stream->bit_ix = 0;
    stream->size = 0;
}

// Grow backing buffer so that it fits at least 'needed' data after the cursor
static void stream_reserve(Stream *stream, size_t reserve) {
    // This must always be true
    check_or(stream->size <= stream->capacity) return;

    size_t capacity_needed = stream->cursor + reserve;

    // Check if we dont need to grow
    if (capacity_needed <= stream->capacity) return;

    // Check if reallocating is possible
    check_or(stream->mem) return;

    // Compute new capacity, growing in powers of two
    size_t new_capacity = MAX(stream->capacity, 16);
    while (capacity_needed > new_capacity) new_capacity *= 2;

    // Allocate new data and copy existing data to the new buffer
    stream->buffer = mem_realloc(stream->mem, stream->buffer, stream->size, new_capacity);
    stream->capacity = new_capacity;
}

// ===================== Byte Read / Write ============================

// Write an aligned byte from the stream
static void stream_write_u8(Stream *stream, u8 data) {
    // Reset bit index (algins to the next byte)
    stream->bit_ix = 0;
    stream_reserve(stream, 1);

    // Stop when an error occurred while reserving capacity
    if (error) return;

    // Write Byte to the buffer
    stream->buffer[stream->cursor++] = data;

    // Update buffer size
    if (stream->size < stream->cursor) stream->size = stream->cursor;
}

// Read an aligned byte from the stream
static u8 stream_read_u8(Stream *stream) {
    // Reset bit index (algins to the next byte)
    stream->bit_ix = 0;

    // When no more byes remain, trigger an error
    // and return some default value
    check_or(!stream_eof(stream)) return 0;

    // Read byte
    return stream->buffer[stream->cursor++];
}

// Return the next 'size' bytes as a buffer
// - Returns an error if
static Buffer stream_read_buffer(Stream *stream, size_t size) {
    size_t max_size = stream_remaining(stream);

    // Throw error if the size does not match
    check_or(size <= max_size) size = max_size;

    // Create a buffer for the remaining data
    Buffer ret = buf_from(stream->buffer + stream->cursor, size);

    // Advance the read cursor
    stream->bit_ix = 0;
    stream->cursor += size;
    return ret;
}

// Write the contents of the buffer to the stream
static void stream_write_buffer(Stream *stream, Buffer buffer) {
    stream_reserve(stream, buffer.size);
    for (size_t i = 0; i < buffer.size; ++i) {
        stream_write_u8(stream, buffer.data[i]);
    }
}

// Read 'size' bytes of 'data' to the stream
static void stream_read_bytes(Stream *stream, size_t count, u8 *data) {
    check_or(stream->cursor + count <= stream->size) return;
    for (size_t i = 0; i < count; ++i) {
        data[i] = stream_read_u8(stream);
    }
}

// Write 'size' bytes of 'data' to the stream
static void stream_write_bytes(Stream *stream, size_t size, u8 *data) {
    stream_write_buffer(stream, buf_from(data, size));
}

// Read two bytes as a little endian u16 from the stream
static u16 stream_read_u16(Stream *stream) {
    u16 out = 0;
    out |= (u16)stream_read_u8(stream) << (0 * 8);
    out |= (u16)stream_read_u8(stream) << (1 * 8);
    return out;
}

// Write two bytes as a little endian u16 from the stream
static void stream_write_u16(Stream *stream, u16 data) {
    stream_write_u8(stream, (data >> (0 * 8)) & 0xff);
    stream_write_u8(stream, (data >> (1 * 8)) & 0xff);
}

// Read four bytes as a little endian u32 from the stream
static u32 stream_read_u32(Stream *stream) {
    u32 out = 0;
    out |= (u32)stream_read_u8(stream) << (0 * 8);
    out |= (u32)stream_read_u8(stream) << (1 * 8);
    out |= (u32)stream_read_u8(stream) << (2 * 8);
    out |= (u32)stream_read_u8(stream) << (3 * 8);
    return out;
}

// Write four bytes as a little endian u32 to the stream
static void stream_write_u32(Stream *stream, u32 data) {
    stream_write_u8(stream, (data >> (0 * 8)) & 0xff);
    stream_write_u8(stream, (data >> (1 * 8)) & 0xff);
    stream_write_u8(stream, (data >> (2 * 8)) & 0xff);
    stream_write_u8(stream, (data >> (3 * 8)) & 0xff);
}

// ========================== BITS =================

// Write a single bit from the stream
static void stream_write_bit(Stream *stream, bool bit) {
    // bit_ix 0 -> Read Byte,
    //
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

static void stream_test_bits(Memory *mem) {
    Stream *stream = stream_new(mem);
    check(stream_size(stream) == 0);
    check(stream_remaining(stream) == 0);

    stream_write_bit(stream, 1);

    check(stream_size(stream) == 1);
    check(stream_remaining(stream) == 0);

    stream_write_bits(stream, 2, 0b10);

    check(stream_size(stream) == 1);
    check(stream_remaining(stream) == 0);

    stream_restart(stream);

    check(stream_size(stream) == 1);
    check(stream_remaining(stream) == 1);

    check(stream_read_u8(stream) == 0b101);

    stream_restart(stream);
    stream_write_bits_be(stream, 16, 0b1111000000000001);

    stream_restart(stream);
    check(stream_read_bits(stream, 16) == 0b1000000000001111);

    stream_restart(stream);
    check(stream_read_bits_be(stream, 16) == 0b1111000000000001);
}

// ====================== File IO ==============================

// Write all file contents to the stream
static void stream_write_from_file(Stream *stream, File *input) {
    Buffer buffer = buf_stack(4 * 1024);
    for (;;) {
        size_t used = os_read(input, buffer);
        stream_write_buffer(stream, buf_take(buffer, used));
        if (used < buffer.size) break;
        if (error) break;
    }
}

// Testing
static void stream_test(Memory *mem) {
    stream_test_bits(mem);

    // Basic test
    Stream *stream = stream_new(mem);
    stream_write_u8(stream, 1);
    stream_write_u8(stream, 2);
    stream_write_u8(stream, 3);
    check(stream->size == 3);
    check(stream->cursor == stream->size);
    check(stream->size <= stream->capacity);
    check(stream->buffer[0] == 1);
    check(stream->buffer[1] == 2);
    check(stream->buffer[2] == 3);

    // Reading back data
    stream_restart(stream);
    check(stream_read_u8(stream) == 1);
    check(stream_read_u8(stream) == 2);
    check(stream_read_u8(stream) == 3);
    check(stream_eof(stream));

    size_t cur = stream_cursor(stream);
    check(cur == 3);

    stream_write_u32(stream, 0x12345678);
    check(stream_cursor(stream) == 7);

    stream_seek(stream, cur);
    check(stream_read_u8(stream) == 0x78);
    check(stream_read_u8(stream) == 0x56);
    check(stream_read_u8(stream) == 0x34);
    check(stream_read_u8(stream) == 0x12);
    check(stream_eof(stream));

    stream_clear(stream);
    check(stream_eof(stream));
    check(stream->cursor == 0);
    check(stream->bit_ix == 0);

    stream_write_bit(stream, 1);
    check(stream->cursor == 1);
    check(stream->bit_ix == 1);

    stream_write_bit(stream, 0);
    stream_write_bit(stream, 1);
    stream_write_bit(stream, 1);
    stream_write_bit(stream, 0);
    stream_write_bit(stream, 0);
    stream_write_bit(stream, 0);
    stream_write_bit(stream, 1);
    check(stream->cursor == 1);
    check(stream->bit_ix == 0);

    // Extra
    stream_write_bit(stream, 1);
    stream_write_bit(stream, 1);

    stream_restart(stream);
    check(stream_read_u8(stream) == 0b10001101);
    check(stream_read_u8(stream) == 0b00000011);
    check(stream_eof(stream));

    stream_restart(stream);
    check(stream_read_bit(stream) == 1);
    check(stream_read_bit(stream) == 0);
    check(stream_read_bit(stream) == 1);
    check(stream_read_bit(stream) == 1);
    check(stream_read_bit(stream) == 0);
    check(stream_read_bit(stream) == 0);
    check(stream_read_bit(stream) == 0);
    check(stream_read_bit(stream) == 1);
    check(stream_read_bit(stream) == 1);
    check(stream_read_bit(stream) == 1);
    check(stream_eof(stream));
    stream_restart(stream);

    check(stream_read_bits(stream, 10) == 0b1110001101);
    check(stream_eof(stream));

    // Write bits
    stream_clear(stream);
    stream_write_bits(stream, 7, 0b11111111);
    stream_write_bits(stream, 7, 0b10000000);
    stream_write_bits(stream, 7, 0b11111111);
    stream_write_bits(stream, 7, 0b10101010);
    stream_restart(stream);
    check(stream_read_bits(stream, 7) == 0b01111111);
    check(stream_read_bits(stream, 7) == 0b00000000);
    check(stream_read_bits(stream, 7) == 0b01111111);
    check(stream_read_bits(stream, 7) == 0b00101010);
    check(stream_eof(stream));
    check(stream->cursor == 4);
    check(stream->size == 4);
    check(stream->bit_ix == 4);

    // Another test
    stream_clear(stream);

    u32 data[] = {0b11000001111100001111000111001101};
    Stream bits = stream_from_buffer(buf_from(data, sizeof(data)));
    check(stream_read_bits(&bits, 1) == 0b1);
    check(stream_read_bits(&bits, 1) == 0b0);
    check(stream_read_bits(&bits, 2) == 0b11);
    check(stream_read_bits(&bits, 2) == 0b00);
    check(stream_read_bits(&bits, 3) == 0b111);
    check(stream_read_bits(&bits, 3) == 0b000);
    check(stream_read_bits(&bits, 4) == 0b1111);
    check(stream_read_bits(&bits, 4) == 0b0000);
    check(stream_read_bits(&bits, 5) == 0b11111);
    check(stream_read_bits(&bits, 5) == 0b00000);
    check(stream_read_bits(&bits, 2) == 0b11);
    check(bits.bit_ix == 0);
    check(bits.cursor == 4);

    stream_restart(&bits);
    check(stream_read_bits(&bits, 7) == 0b1001101);
    check(stream_read_bits(&bits, 7) == 0b1100011);
    check(bits.bit_ix == 6);

    // Should skip the 2 bits
    check(stream_read_u8(&bits) == 0b11110000);
    check(bits.bit_ix == 0);

    check(stream_read_bits(&bits, 1) == 1);
    check(stream_read_bits(&bits, 1) == 0);

    stream_restart(&bits);
    check(stream_read_bits(&bits, 32) == 0b11000001111100001111000111001101);
    check(stream_eof(&bits));

    stream_restart(&bits);
    check(stream_read_u32(&bits) == 0b11000001111100001111000111001101);
    check(stream_eof(&bits));

    stream_clear(&bits);
    stream_write_bits(&bits, 4, 0b0000);
    stream_write_bits(&bits, 4, 0b1111);
    stream_write_bits(&bits, 1, 0b1);
    stream_write_bits(&bits, 1, 0b0);

    stream_restart(&bits);
    check(stream_read_bits(&bits, 4) == 0b0000);
    check(stream_read_bits(&bits, 4) == 0b1111);
    check(stream_read_bits(&bits, 1) == 0b1);
    check(stream_read_bits(&bits, 1) == 0b0);
}

// NOTE: make fmt also use stream?
// static void fmt_int(Stream *fmt, u64 value) {
//     fmt_s(fmt, "Hello");
//     fmt_s(fmt, "World");
//     fmt_eol(fmt);
// }

// static void fmt_str(Stream *fmt, char *str) {
//     stream_write_buffer(fmt, str_buf(str));
// }

// typedef struct {
//     u8 buffer[64];
//     Stream stream;
//     Fmt_Options opt;
// } Fmt2;

// static void fmt_int(Stream *fmt, Fmt_Options opt, bool sign, u64 value) {
//     u8 buffer[64];
//     Fmt2 fmt2;
//     fmt2.stream = stream_from_buffer(buf_from(fmt2.buffer, sizeof(fmt2.buffer)));

//     ({
//         Memory *fmt_mem = mem_new();
//         Stream *fmt_str = stream_new(fmt_mem);
//         Fmt_Options opt = {};
//         os_write(os_stdout(), stream_as_buffer(fmt_str));
//         mem_free(fmt_mem);
//     });
// }
