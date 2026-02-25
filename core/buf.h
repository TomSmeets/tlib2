// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// buf.h - Generic data operations on buffers
#pragma once
#include "type.h"

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

typedef struct {
    void **items;
    size_t count;
} List;
#define LIST(T, ...) ((T *[]){__VA_ARGS__, 0})

static Buffer buf_from(void *data, size_t size) {
    return (Buffer){data, size};
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
