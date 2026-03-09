// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// test.c - Run all unit tests
#define main _main
#include "arg.h"
#include "base64.h"
#include "crc.h"
#include "deflate.h"
#include "error2.h"
#include "fmt.h"
#include "gzip.h"

void os_main(u32 argc, char **argv){}

int LLVMFuzzerTestOneInput(u8 *data, size_t size) {
    Buffer input = {data,size};
    Memory *mem = mem_new();
    assert(buf_eq(base64_decode(mem, base64_encode(mem, input)), input));
    assert(deflate_test_buf(mem, input));
    assert(gzip_test_buf(mem, input));
    crc_compute(input);
    mem_free(mem);
    return 0;
}
