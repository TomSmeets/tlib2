// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// str.h - String helper functions
#pragma once
#include "type.h"
#include "buf.h"

static bool str_eq(char *a, char *b) {
    for (;;) {
        if (*a == 0 && *b == 0) return 1;
        if (*a != *b) return 0;
        a++;
        b++;
    }
}

// Get length of the zero terminated string
static u32 str_len(char *str) {
    u32 len = 0;
    while (*str++) len++;
    return len;
}

// Convert a zero terminated string to a buffer
static Buffer str_buf(char *str) {
    return (Buffer){
        str,
        str_len(str),
    };
}
