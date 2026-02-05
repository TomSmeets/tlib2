// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// test.c - Run all unit tests
#include "fmt.h"
#include "os.h"
#include "base64.h"

void os_main(u32 argc, char **argv) {
    test_fmt();
    base64_test();
    os_exit(0);
}
