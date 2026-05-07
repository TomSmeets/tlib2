// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// test.c - Run all unit tests
#include "base64.h"
#include "crc.h"
#include "deflate.h"
#include "fmt.h"
#include "gzip.h"

int LLVMFuzzerTestOneInput(u8 *data, size_t size) {
    Buffer input = {data, size};
    Memory *mem = mem_new();

    Buffer output = base64_decode(mem, input);
    error = 0;

    check(buf_eq(base64_decode(mem, base64_encode(mem, input)), input));
    deflate_test_buf(mem, input);
    gzip_test_buf(mem, input);
    crc_compute(input);
    mem_free(mem);
    check(chunk_alloc_size == 0);
    if (error) __builtin_trap();
    mem_tmp_free();
    mem_perm_free();
    return 0;
}
