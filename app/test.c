// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// test.c - Run all unit tests
#include "arg.h"
#include "base64.h"
#include "crc.h"
#include "fmt.h"
#include "gzip.h"
#include "huffman.h"
#include "os.h"
#include "stream.h"

void os_main(u32 argc, char **argv) {
    fmt_s(fout, "Running tests...\n");
    fmt_test();
    base64_test();
    arg_test();
    gzip_test();
    crc_test();
    stream_test();
    huffman_test();
    fmt_s(fout, "Success!\n");
    os_exit(0);
}
