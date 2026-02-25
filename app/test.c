// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// test.c - Run all unit tests
#include "arg.h"
#include "base64.h"
#include "crc.h"
#include "deflate_write.h"
#include "fmt.h"
#include "gzip.h"
#include "huffman_code.h"
#include "huffman_tree.h"
#include "os.h"
#include "stream.h"

#define TEST(name)                                                                                                                                   \
    fmt_s(fout, "Running " #name "\n");                                                                                                              \
    try(name)

bool test_test(void) {
    TEST(base64_test());
    TEST(fmt_test());
    TEST(arg_test());
    TEST(crc_test());
    TEST(stream_test());
    TEST(huffman_code_test());
    TEST(huffman_tree_test());
    TEST(gzip_test());
    TEST(deflate_write_test());
    return ok();
}

void os_main(u32 argc, char **argv) {
    if (!test_test()) error_exit();
    fmt_s(fout, "Success!\n");
    os_exit(0);
}
