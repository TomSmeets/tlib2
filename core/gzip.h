#pragma once
#include "mem.h"
#include "fmt.h"
#include "base64.h"

// static Buffer gzip_extract(Memory *mem, Buffer data) {
// }



static void gzip_test(void) {
    Memory *mem = mem_new();
    char *target =
        "hello hello world hello hello"
        "hello hello world hello hello"
        "hello hello world hello hello"
        "hello hello world hello hello"
        "hello hello wor\nld hello hello"
        "hello hello world hello hello"
        "hello hello world hello hello"
        "hello hello world hello hello"
        "hello hello world hello hello"
        "hello hello world hello hello"
        "hello hello world hello hello"
        "hello hello world hello hello"
        "hello hello world hello hello";
    fmt_hexdump(fout, str_buf(target));
    // Buffer compressed = base64_decode(mem, str_buf("H4sIAAAAAAAAA8tIzcnJV8gAk+X5RTkpUDaY5AIAmdZcBR4AAAA="));
}
