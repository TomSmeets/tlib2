// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// test.c - Run all unit tests
#include "base64.h"
#include "cli.h"
#include "crc.h"
#include "deflate.h"
#include "fmt.h"
#include "gzip.h"
// #include "hot.h"
#include "huffman_code.h"
#include "huffman_tree.h"
#include "os.h"
#include "read.h"
// #include "tlang/tlang.h"
#include "os_main.h"
#include "tom.h"

#define TEST(FCN, ...) \
    ({ \
        print(F_Green, "Testing ", F_Yellow, #FCN); \
        FCN; \
        if (error) os_exit(); \
        debug(error); \
    })

static void os_main(void) {
    // Run tests
    TEST(test_ptr());
    TEST(test_mem());
    TEST(test_read());
    TEST(test_write());
    TEST(test_tom());
    // TEST(test_tlang());
    TEST(test_base64());
    TEST(test_fmt());
    TEST(test_cli_arg());
    TEST(test_cli());
    TEST(test_crc());
    TEST(test_huffman_code());
    TEST(test_huffman_tree());
    TEST(test_deflate());
    TEST(test_gzip());
    if (!error) print("Success!");
    os_exit();
}
