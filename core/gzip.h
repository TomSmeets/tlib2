#pragma once
#include "base64.h"
#include "bits.h"
#include "deflate.h"
#include "fmt.h"
#include "mem.h"
#include "stream.h"

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
    bool ftext = (flags >> 0) & 1;
    bool fhcrc = (flags >> 1) & 1;
    bool fextra = (flags >> 2) & 1;
    bool fname = (flags >> 3) & 1;
    bool fcomment = (flags >> 4) & 1;
    if (fextra != 0) return out;

    u32 mtime = stream_read_u32(&stream);
    u8 xfl = stream_read_u8(&stream);
    if (xfl != 0) return out;

    u8 os = stream_read_u8(&stream);
    while (fname && stream_read_u8(&stream));
    while (fcomment && stream_read_u8(&stream));

    if (fhcrc) {
        u16 crc = stream_read_u16(&stream);
    }

    out = deflate_read(mem, &stream);

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
    Buffer t0_target = base64_decode(mem, str_buf("GnSwX91w7Z9EqpaZeyPCIQ=="));
    Buffer t0_in = base64_decode(mem, str_buf("H4sICHPOkWkAA2RhdGEAkyrZEH+34O18l1XTZlYrH1IEAFve5PUQAAAA"));
    // Buffer t0_out = gzip_read(mem, t0_in);
    // assert(buf_eq(t0_out, t0_target));

    Buffer t1_target = str_buf("hello hello world hello hello");
    Buffer t1_in = base64_decode(mem, str_buf("H4sIAAAAAAAAA8tIzcnJV8gAk+X5RTkpUDaY5AIAmdZcBR4AAAA="));
    // Buffer t1_out = gzip_read(mem, t1_in);
    // assert(buf_eq(t1_out, t1_target));
}
