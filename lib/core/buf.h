// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// buf.h - Generic data operations on buffers
#pragma once
#include "type.h"
#include "ptr.h"
#include "chr.h"


// A simple range of data in memory
typedef struct {
    u8 *data;
    size_t size;
} Buffer;

#define BUFFER(T, ...) ((Buffer){(T[]){__VA_ARGS__}, sizeof((T[]){__VA_ARGS__})})

// Stack allocate a buffer of a given size
#define buf_stack(SIZE) ((Buffer){(u8[(SIZE)]){}, (SIZE)})
#define buf_from(DATA, SIZE) ((Buffer){(u8 *)(DATA), (SIZE)})
#define buf_from_struct(S) buf_from((S), sizeof(*(S)))

// An empty buffer
#define buf_null() ((Buffer){0})

// Check if two buffers contain the same data
static bool buf_eq(Buffer a, Buffer b) {
    if (a.size != b.size) return 0;
    return ptr_eq(a.data, b.data, a.size);
}

// Check if buffer starts with the same data
static bool buf_starts_with(Buffer buf, Buffer start) {
    if (buf.size < start.size) return false;
    return ptr_eq(buf.data, start.data, start.size);
}

// Return the first 'size' bytes of the buffer
static Buffer buf_take(Buffer a, size_t size) {
    if (size > a.size) size = a.size;
    return (Buffer){a.data, size};
}

// Discard the first 'size' bytes of the buffer, and return the rest
static Buffer buf_drop(Buffer a, size_t size) {
    if (size > a.size) size = a.size;
    return (Buffer){a.data + size, a.size - size};
}

// Return the last 'size' bytes of the buffer
static Buffer buf_take_end(Buffer buf, size_t size) {
    if (size > buf.size) size = buf.size;
    return buf_drop(buf, buf.size - size);
}

// Discard the last 'size' bytes of the buffer
static Buffer buf_drop_end(Buffer buf, size_t size) {
    if (size > buf.size) size = buf.size;
    return buf_take(buf, buf.size - size);
}

// Create a buffer starting at the start offset with the given size
static Buffer buf_slice(Buffer buf, size_t start, size_t size) {
    return buf_take(buf_drop(buf, start), size);
}

// Count the number of matching bytes from the start
static size_t buf_match_len(Buffer a, Buffer b) {
    for (size_t i = 0;; ++i) {
        // Reached the end of one of the buffers
        if (i == a.size || i == b.size) return i;

        // No more matching bytes
        if (a.data[i] != b.data[i]) return i;
    }
}

// Trim whitespace from the end of the buffer
static Buffer buf_trim_end(Buffer buf) {
    while (1) {
        // Buffer is empty
        if (buf.size == 0) break;

        // Buffer does not have a whitespace at the end
        if (!chr_is_whitespace(buf.data[buf.size - 1])) break;

        // Reduce buffer size
        buf.size--;
    }
    return buf;
}

// Trim whitespace from the start of the buffer
static Buffer buf_trim_start(Buffer buf) {
    while (1) {
        // Buffer is empty
        if (buf.size == 0) break;

        // Buffer does not have a whitespace at the end
        if (!chr_is_whitespace(buf.data[0])) break;

        // Reduce buffer size
        buf.size--;
        buf.data++;
    }
    return buf;
}

// Trim whitespace from start and end
static Buffer buf_trim(Buffer buf) {
    buf = buf_trim_start(buf);
    buf = buf_trim_end(buf);
    return buf;
}
