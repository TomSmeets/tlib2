// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// str.h - String helper functions
#pragma once
#include "buf.h"
#include "type.h"

// Compare two strings for byte equality
static bool str_eq(char *a, char *b) {
    for (;;) {
        if (*a == 0 && *b == 0) return 1;
        if (*a != *b) return 0;
        a++;
        b++;
    }
}

// Get length of the zero terminated string
static size_t str_len(char *str) {
    size_t len = 0;
    while (*str++) len++;
    return len;
}

// Convert a zero terminated string to a buffer with a length
static Buffer str_buf(char *str) {
    return buf_from(str, str_len(str));
}

// Compare two strings
// Returns:
//   -1 if A < B
//    0 if A == B
//   +1 if A > B
static i32 str_compare(char *a, char *b) {
    for (;;) {
        if (*a == 0 && *b == 0) return 0;
        if (*a < *b) return -1;
        if (*a > *b) return 1;
        a++;
        b++;
    }
}
