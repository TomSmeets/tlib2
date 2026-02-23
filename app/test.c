// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// test.c - Run all unit tests
#include "arg.h"
#include "base64.h"
#include "crc.h"
#include "fmt.h"
#include "gzip.h"
#include "huffman.h"
#include "os.h"
#include "stream.h"

bool test_test(void) {
    try(base64_test());
    try(fmt_test());
    try(arg_test());
    try(crc_test());
    try(stream_test());
    try(huffman_test());
    try(gzip_test());
    return ok();
}

void os_main(u32 argc, char **argv) {
    fmt_s(fout, "Running tests...\n");
    if(!test_test()) error_exit();
    fmt_s(fout, "Success!\n");
    os_exit(0);
}
