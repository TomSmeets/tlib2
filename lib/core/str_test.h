#pragma once
#include "str.h"
#include "error.h"

static void test_str(void) {
    char msg[] = "Hello World";

    // str_eq
    check(str_eq(msg, "Hello World"));
    check(!str_eq(msg, "Hello World X"));
    check(!str_eq(msg, "Hello Worl"));
    check(!str_eq(msg, ""));
    check(!str_eq(msg, "X"));
    check(str_eq("", ""));

    // str_len
    check(str_len("") == 0);
    check(str_len("X") == 1);
    check(str_len("Hello World!") == 12);
    check(str_len(msg) == 11);

    // str_buf
    check(buf_eq(str_buf("Hello"), buf_from("Hello", 5)));

    // str_compare
    check(str_compare("aaa", "aaa") == 0);
    check(str_compare("aaa", "aab") == -1);
    check(str_compare("aab", "aaa") == 1);
    check(str_compare("aab", "baa") == -1);
    check(str_compare("aab", "baa") == -1);
    check(str_compare("", "") == 0);
    check(str_compare("a", "") == 1);
    check(str_compare("", "a") == -1);

    // str_contains_chr
    check(str_contains_chr(msg, 'H'));
    check(str_contains_chr(msg, 'o'));
    check(!str_contains_chr(msg, 'x'));
    check(!str_contains_chr(msg, 'h'));
    check(!str_contains_chr("", 'x'));
}
