// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// macro_test.h - Tests macro.h
#pragma once
#include "error.h"
#include "macro.h"
#include "str.h"

typedef struct {
    u8 a;
    u16 b;
    u32 c;
    u64 d;
} macro_test_struct;

static void test_macro(void) {
    // offset_of
    check(offset_of(macro_test_struct, a) == 0);
    check(offset_of(macro_test_struct, b) == 2);
    check(offset_of(macro_test_struct, c) == 4);
    check(offset_of(macro_test_struct, d) == 8);

    check(str_eq(TO_STRING(123), "123"));
    check(str_eq(TO_STRING(1 + 2), "1 + 2"));
    check(str_eq(TO_STRING(__LINE__), "24"));

    // 11 chars + zero term
    char test[] = "Hello World";
    check(array_count(test) == 12);

    // should consider size of the type
    void *ptrs[] = { 0, &test_macro };
    check(array_count(ptrs) == 2);

    // One item
    int one[] = {1};
    check(array_count(one) == 1);

    // Zero items
    int zero[] = {};
    check(array_count(zero) == 0);

    // MIN/MAX
    check(MIN(123, 456) == 123);
    check(MAX(123, 456) == 456);
    check(MIN(-123, -456) == -456);
    check(MAX(-123, -456) == -123);

    // REPEAT
    int x = 0;
    #define INC(I) x += I
    REPEAT(INC, 1, 2, 3, 4);
    check(x == 1 + 2 + 3 + 4);
    #undef INC
}
