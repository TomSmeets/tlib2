// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// ptr.h - Methods on bytes memzero/memset/memcpy
#pragma once
#include "type.h"

// Copy a non-overlapping memory region from src to dst
static void ptr_copy(void *dst, void *src, size_t size) {
    u8 *p_src = src;
    u8 *p_dst = dst;
    while (size--) *p_dst++ = *p_src++;
}

// Clear memory to zero
static void ptr_zero(void *dst, size_t size) {
    u8 *p_dst = dst;
    while (size--) *p_dst++ = 0;
}

// Check two memory regions for equality
static bool ptr_eq(void *a, void *b, size_t size) {
    u8 *p_a = a;
    u8 *p_b = b;
    while (size--) {
        if (*p_a++ != *p_b++) return 0;
    }
    return 1;
}
