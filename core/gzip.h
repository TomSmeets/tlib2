#pragma once
#include "mem.h"
#include "fmt.h"
#include "base64.h"

static void gzip_extract(Memory *mem, Buffer data) {
}



static void gzip_test(void) {
    Memory *mem = mem_new();
    fmt_s(fout, "Target:\n");
    char *target = "hello hello world hello hello";
    fmt_hexdump(fout, str_buf(target));

    fmt_s(fout, "Compressed:\n");
    Buffer compressed = base64_decode(mem, str_buf("H4sIAAAAAAAAA8tIzcnJV8gAk+X5RTkpUDaY5AIAmdZcBR4AAAA="));
    fmt_hexdump(fout, compressed);

    // Do something
    gzip_extract(mem, compressed);
}
