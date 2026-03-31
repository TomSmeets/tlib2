// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// test.c - Run all unit tests
#include "arg.h"
#include "base64.h"
#include "cli.h"
#include "crc.h"
#include "deflate.h"
#include "fmt.h"
#include "gzip.h"
#include "hot.h"
#include "huffman_code.h"
#include "huffman_tree.h"
#include "os.h"
#include "read.h"
#include "tlang/tlang.h"

#define TEST(FCN, ...)                                                                                                                               \
    ({                                                                                                                                               \
        print("Running " #FCN "...");                                                                                                                \
        FCN;                                                                                                                                         \
        if (error) os_exit();                                                                                                                        \
    })

void os_main(u32 argc, char **argv) {
    // Run tests
    TEST(test_mem());
    TEST(test_read());
    TEST(test_write());
    TEST(tlang_test());
    TEST(base64_test());
    TEST(fmt_test());
    TEST(arg_test());
    TEST(test_cli_arg());
    TEST(test_cli());
    TEST(crc_test());
    TEST(huffman_code_test());
    TEST(huffman_tree_test());
    TEST(deflate_test());
    TEST(gzip_test());
    if (!error) print("Success!");
    os_exit();
}
