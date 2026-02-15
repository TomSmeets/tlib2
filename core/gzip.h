#pragma once
#include "base64.h"
#include "bits.h"
#include "fmt.h"
#include "mem.h"
#include "stream.h"
#include "deflate.h"

static Buffer gzip_read(Memory *mem, Buffer data) {
    Stream stream = stream_from(data);
    Buffer out = {};

    u8 magic1 = stream_read_u8(&stream);
    u8 magic2 = stream_read_u8(&stream);
    if (magic1 != 0x1f) return out;
    if (magic2 != 0x8b) return out;

    // Compression Method (8 = gzip)
    u8 method = stream_read_u8(&stream);
    if (method != 0x8) return out;

    u8 flags = stream_read_u8(&stream);
    if (flags != 0) return out;

    u32 mtime = stream_read_u32(&stream);

    u8 xfl = stream_read_u8(&stream);
    if (xfl != 0) return out;

    u8 os = stream_read_u8(&stream);

    deflate_read(mem, &stream);

    // Seek to end
    stream_seek(&stream, stream.size - 8);
    u32 crc = stream_read_u32(&stream);
    u32 isize = stream_read_u32(&stream);
    fmt_sx(fout, "crc: ", crc, "\n");
    fmt_sx(fout, "isize: ", isize, "\n");

    return out;
}

static void gzip_test(void) {
    Memory *mem = mem_new();
    fmt_s(fout, "Type0:\n");
    Buffer t0_target = base64_decode(mem, str_buf("GnSwX91w7Z9EqpaZeyPCIQ=="));
    Buffer t0_in     = base64_decode(mem, str_buf("H4sICHPOkWkAA2RhdGEAkyrZEH+34O18l1XTZlYrH1IEAFve5PUQAAAA"));
    Buffer t0_out    = gzip_read(mem, t0_in);
    assert(buf_eq(t0_out, t0_target));

    fmt_s(fout, "Type1:\n");
    Buffer t1_target = str_buf("hello hello world hello hello");
    Buffer t1_in = base64_decode(mem, str_buf("H4sIAAAAAAAAA8tIzcnJV8gAk+X5RTkpUDaY5AIAmdZcBR4AAAA="));
    Buffer t1_out    = gzip_read(mem, t1_in);
    assert(buf_eq(t1_out, t1_target));
}
