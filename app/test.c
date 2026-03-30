// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// test.c - Run all unit tests
#include "arg.h"
#include "base64.h"
#include "crc.h"
#include "read.h"
#include "deflate.h"
#include "fmt.h"
#include "gzip.h"
#include "hot.h"
#include "huffman_code.h"
#include "huffman_tree.h"
#include "os.h"
#include "stream.h"
#include "tlang/tlang.h"

void os_main(u32 argc, char **argv) {
    Memory *mem = mem_new();

    // Run tests
    mem_test();
    if (error) os_exit();

    test_read();
    if(error) return;
    tlang_test(mem);
    base64_test(mem);
    fmt_test(mem);
    arg_test();
    crc_test();
    stream_test(mem);
    huffman_code_test(mem);
    huffman_tree_test(mem);
    deflate_test(mem);
    gzip_test(mem);

    if (error) os_exit();

    print("Success!");
    os_exit();
}
