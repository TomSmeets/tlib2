// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#pragma once
#include "type.h"

static bool str_eq(const char *a, const char *b) {
    for (;;) {
        if (*a == 0 && *b == 0) return 1;
        if (*a != *b) return 0;
        a++;
        b++;
    }
}

// Number of chars in a zero terminated string
static u32 str_len(const char *str) {
    u32 len = 0;
    while (*str++) len++;
    return len;
}
