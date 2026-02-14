#pragma once
#include "base64.h"
#include "fmt.h"
#include "mem.h"
#include "bits.h"
#include "stream.h"


static void gzip_extract(Memory *mem, Buffer data) {
    Stream stream = stream_from(data);

    u8 magic1 = stream_read(&stream, 1);
    u8 magic2 = stream_read(&stream, 1);
    u8 cm = stream_read(&stream, 1);
    u8 flags = stream_read(&stream, 1);
    u32 mtime = stream_read(&stream, 4);
    u8 xfl = stream_read(&stream, 1);
    u8 os = stream_read(&stream, 1);
    fmt_sx(fout, "magic1: ", magic1, "\n");
    fmt_sx(fout, "magic2: ", magic2, "\n");
    fmt_sx(fout, "cm: ", cm, "\n");
    fmt_sx(fout, "flags: ", flags, "\n");
    fmt_sx(fout, "mtime: ", mtime, "\n");
    fmt_sx(fout, "xfl: ", xfl, "\n");
    fmt_sx(fout, "os: ", os, "\n");

    // Seek to end
    stream_seek(&stream, stream.size - 8);
    u32 crc   = stream_read(&stream, 4);
    u32 isize = stream_read(&stream, 4);
    fmt_sx(fout, "crc: ",crc, "\n");
    fmt_sx(fout, "isize: ",isize, "\n");

    // Deflate
    assert(magic1 == 0x1f);
    assert(magic2 = 0x8b);
    assert(cm == 8);
    assert(os == 3);
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
