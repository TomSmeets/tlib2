// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// test.c - Run all unit tests
#include "arg.h"
#include "base64.h"
#include "crc.h"
#include "deflate.h"
#include "fmt.h"
#include "gzip.h"
#include "hot.h"
#include "huffman_code.h"
#include "huffman_tree.h"
#include "os.h"
#include "stream.h"

#define TEST(name)                                                                                                                                   \
    print("Running " #name);                                                                                                                         \
    try(name)

static void test_test(Memory *mem) {
    base64_test(mem);
    fmt_test(mem);
    arg_test();
    crc_test();
    stream_test(mem);
    huffman_code_test(mem);
    huffman_tree_test(mem);
    deflate_test(mem);
    gzip_test(mem);
}

void os_main(u32 argc, char **argv) {
    Memory *mem = mem_new();

    // Run tests
    base64_test(mem);
    fmt_test(mem);
    arg_test();
    crc_test();
    stream_test(mem);
    huffman_code_test(mem);
    huffman_tree_test(mem);
    deflate_test(mem);
    gzip_test(mem);

    // Release memory
    mem_free(mem);

    // Check result
    if (error) {
        for (u32 i = 0; i < error; ++i) {
            print(error_stack[i].file, ":", error_stack[i].line, ":", error_stack[i].function, ": Error ", error_stack[i].expr, " failed");
        }
        os_exit(1);
    } else {
        print("Success!");
        os_exit(0);
    }
}
