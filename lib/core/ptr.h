// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// ptr.h - Methods on bytes memzero/memset/memcpy
#pragma once
#include "error.h"
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

// Reverse bytes
static void ptr_reverse(void *data, size_t size) {
    u8 *start = data;
    u8 *end = data + size - 1;
    for (size_t i = 0; i < size / 2; ++i) {
        u8 *p1 = start + i;
        u8 *p2 = end - i;
        u8 b1 = *p1;
        u8 b2 = *p2;
        *p1 = b2;
        *p2 = b1;
    }
}

static void test_ptr(void) {
    check(ptr_eq("Hello", "Hello", 5) == 1);
    check(ptr_eq("Hello", "HellX", 5) == 0);
    check(ptr_eq("", "", 0) == 1);

    {
        u8 buf[4] = "ABCD";
        ptr_reverse(buf, 4);
        check(ptr_eq(buf, "DCBA", 4));
    }

    {
        u8 buf[5] = "ABCDE";
        ptr_reverse(buf, 5);
        check(ptr_eq(buf, "EDCBA", 5));
    }
}
