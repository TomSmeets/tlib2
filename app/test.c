// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// test.c - Run all unit tests
#include "fmt.h"
#include "os.h"

void os_main(u32 argc, char **argv) {
    test_fmt();
    os_exit(0);
}
