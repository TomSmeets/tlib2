// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// buf.h - Generic data operations on buffers
#pragma once
#include "type.h"

typedef struct {
    void *data;
    u64 size;
} Buffer;

static void std_memcpy(u8 *restrict dst, const u8 *restrict src, u64 size) {
    while (size--) *dst++ = *src++;
}

static void std_memmove(u8 *dst, const u8 *src, u64 size) {
    while (size--) *dst++ = *src++;
}

static void std_memzero(u8 *dst, u64 size) {
    while (size--) *dst++ = 0;
}

static void std_memset(u8 *dst, u8 value, u64 size) {
    while (size--) *dst++ = value;
}

static bool std_memcmp(const u8 *restrict a, const u8 *restrict b, u64 size) {
    while (size--) {
        if (*a++ != *b++) return false;
    }
    return true;
}

static bool buf_starts_with(Buffer buf, Buffer start) {
    if (buf.size < start.size) return false;
    return std_memcmp(buf.data, start.data, start.size);
}
