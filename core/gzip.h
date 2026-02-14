#pragma once
#include "base64.h"
#include "bits.h"
#include "fmt.h"
#include "mem.h"
#include "stream.h"

static bool gzip_extract(Memory *mem, Buffer data) {
    Stream stream = stream_from(data);

    u8 magic1 = stream_read_u8(&stream);
    u8 magic2 = stream_read_u8(&stream);
    if (magic1 != 0x1f) return 0;
    if (magic2 != 0x8b) return 0;

    // Compression Method (8 = gzip)
    u8 method = stream_read_u8(&stream);
    if (method != 0x8) return 0;

    u8 flags = stream_read_u8(&stream);
    if (flags != 0) return 0;

    u32 mtime = stream_read_u32(&stream);

    u8 xfl = stream_read_u8(&stream);
    if (xfl != 0) return 0;

    u8 os = stream_read_u8(&stream);

    // Seek to end
    stream_seek(&stream, stream.size - 8);
    u32 crc = stream_read_u32(&stream);
    u32 isize = stream_read_u32(&stream);
    fmt_sx(fout, "crc: ", crc, "\n");
    fmt_sx(fout, "isize: ", isize, "\n");
    return 1;
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
    assert(gzip_extract(mem, compressed));
}
