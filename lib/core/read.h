// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// read.h - Reader
#pragma once
#include "type.h"
#include "buf.h"

typedef struct {
    Buffer buffer;
    size_t bytes_read;
    size_t bits_read;
} Read;

// Create a new reader that reads from a buffer
static Read read_from(Buffer buffer) {
    return (Read){.buffer = buffer};
}

// Return true if no more bytes are able to be read
static bool read_eof(Read *read) {
    return read->bytes_read >= read->buffer.size;
}

// Read a single byte
static u8 read_u8(Read *read) {
    check_or(read->bits_read < read->buffer.size) return 0;
    return read->buffer.data[read->bytes_read++];
}

// Read a single bit
static u8 read_bit(Read *read) {
    // Consume next byte
    if (read->bits_read == 0) read_u8(read);

    // Ensure there is data to read
    check(read->bytes_read > 0);
    if(error) return 0;

    u8 byte = read->buffer.data[read->bytes_read - 1];
    if(read->bits_read == 7) read->bits_read = 0;
}

