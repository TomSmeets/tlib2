// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// buf.h - Generic data operations on buffers
#pragma once
#include "type.h"

#define buf_stack(size) (Buffer){(u8[1024]){}, 1024}

// Copy a non-overlapping memory region from src to dst
static void mem_copy(void *restrict dst, void *restrict src, size_t size) {
    if (dst == src || size == 0) return;

    u8 *restrict p_src = src;
    u8 *restrict p_dst = dst;
    while (size--) *(p_dst++) = *(p_src++);
}

// Copy a overlapping memory region from src to dst
static void mem_move(void *dst, void *src, size_t size) {
    if (dst == src || size == 0) return;

    if (dst < src && dst + size >= src) {
        // Move data from start to end
        // [ dst .. src ]
        u8 *p_src = src;
        u8 *p_dst = dst;
        while (size--) *(p_dst++) = *(p_src++);
    } else if (src < dst && src + size >= dst) {
        // Move data from end to start
        // [ src .. dst ]
        u8 *p_src = src + size;
        u8 *p_dst = dst + size;
        while (size--) *(--p_dst) = *(--p_src);
    } else {
        // No overlap -> so we can use a fast memcpy
        u8 *restrict p_src = src;
        u8 *restrict p_dst = dst;
        while (size--) *(p_dst++) = *(p_src++);
    }
}

// Clear memory to zero
static void mem_zero(void *dst, size_t size) {
    u8 *p_dst = dst;
    while (size--) *p_dst++ = 0;
}

// Check two memory regions for equality
static bool mem_eq(void *a, void *b, size_t size) {
    u8 *p_a = a;
    u8 *p_b = b;
    while (size--) {
        if (*(p_a++) != *(p_b++)) return 0;
    }
    return 1;
}

// A simple range of data in memory
typedef struct {
    u8 *data;
    size_t size;
} Buffer;

#define BUFFER(T, ...) ((Buffer){(T[]){__VA_ARGS__}, sizeof((T[]){__VA_ARGS__})})
#define buf_from(DATA, SIZE) ((Buffer){(u8 *)(DATA), (SIZE)})
#define buf_from_struct(S) buf_from((S), sizeof(*(S)))

typedef struct {
    void **items;
    size_t count;
} List;
#define LIST(T, ...) ((T *[]){__VA_ARGS__, 0})

// An invalid buffer
static Buffer buf_null(void) {
    return (Buffer){0};
}

// Check if buffer starts with the same data
static bool buf_starts_with(Buffer buf, Buffer start) {
    if (buf.size < start.size) return false;
    return mem_eq(buf.data, start.data, start.size);
}

// Check if two buffers contain the same data
static bool buf_eq(Buffer a, Buffer b) {
    if (a.size != b.size) return 0;
    return mem_eq(a.data, b.data, a.size);
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
        if (i >= a.size || i >= b.size) return i;
        if (a.data[i] != b.data[i]) return i;
    }
}
