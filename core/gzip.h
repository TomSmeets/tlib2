// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// gzip.h - GZIP decompressor
#pragma once
#include "base64.h"
#include "crc.h"
#include "deflate.h"
#include "error.h"
#include "fmt.h"
#include "mem.h"
#include "stream.h"

static Buffer *gzip_read(Memory *mem, Buffer *input) {
    u8 magic1 = stream_read_u8(input);
    u8 magic2 = stream_read_u8(input);
    try(magic1 == 0x1f && magic2 == 0x8b, "Invalid magic");

    // Compression Method (8 = gzip)
    u8 method = stream_read_u8(input);
    try(method == 0x8, "Invalid compression method, expecting deflate");

    u8 flags = stream_read_u8(input);
    bool ftext = (flags >> 0) & 1;
    bool fhcrc = (flags >> 1) & 1;
    bool fextra = (flags >> 2) & 1;
    bool fname = (flags >> 3) & 1;
    bool fcomment = (flags >> 4) & 1;
    try(fextra == 0, "Unsupported gzip flag");

    u32 mtime = stream_read_u32(input);

    // XFL Compression info:
    // 2 -> Best compression
    // 4 -> Fast compression
    u8 xfl = stream_read_u8(input);

    u8 os = stream_read_u8(input);
    while (fname && stream_read_u8(input));
    while (fcomment && stream_read_u8(input));

    if (fhcrc) {
        u16 crc = stream_read_u16(input);
    }

    Buffer *out = deflate_read(mem, (Buffer){input->buffer + input->cursor, input->size - input->cursor });

    u32 crc = stream_read_u32(input);
    u32 isize = stream_read_u32(input);
    u32 crc_comp = crc_compute(*out);
    try(isize == out->size);
    try(crc == crc_comp);
    try(stream_eof(input));
    return ok(), out;
}

static bool gzip_write(Memory *mem, Buffer input, Stream *output) {
    stream_write_u8(output, 0x1f);
    stream_write_u8(output, 0x8b);
    stream_write_u8(output, 0x08); // method
    stream_write_u8(output, 0); // flags
    stream_write_u32(output, 0); // mtime
    stream_write_u8(output, 0); // xfl
    Buffer *deflated = deflate_write(mem, input);
    try(deflated);
    stream_write_buffer(output, *deflated);
    stream_write_u32(output, crc_compute(input));
    stream_write_u32(output, input.size);
    u32 crc_comp = crc_compute(stream_to_buffer(output));
    return ok();
}

static bool gzip_read_buffer(Memory *mem, Buffer input, Buffer *output) {
    Stream input_stream = stream_from(input);
    Stream *output_stream = stream_new(mem);
    try(gzip_read(mem, &input_stream, output_stream));
    *output = stream_to_buffer(output_stream);
    return ok();
}

static bool gzip_test(void) {
    Memory *mem = mem_new();
    Buffer t0_target = str_buf("hello hello world hello hello\n");
    Buffer t0_in = base64_decode(mem, str_buf("H4sIAAAAAAAAA8tIzcnJV8gAk+X5RTkpUDaY5AIAmdZcBR4AAAA="));
    Buffer t0_out = {};
    try(gzip_read_buffer(mem, t0_in, &t0_out));
    if (0) {
        fmt_hexdump(fout, t0_target);
        fmt_hexdump(fout, t0_out);
    }
    try(buf_eq(t0_out, t0_target));

    // Uses fixed huffman table
    Buffer t1_target = base64_decode(mem, str_buf("GnSwX91w7Z9EqpaZeyPCIQ=="));
    Buffer t1_in = base64_decode(mem, str_buf("H4sICHPOkWkAA2RhdGEAkyrZEH+34O18l1XTZlYrH1IEAFve5PUQAAAA"));
    Buffer t1_out = {};
    try(gzip_read_buffer(mem, t1_in, &t1_out));
    if (0) {
        fmt_hexdump(fout, t1_target);
        fmt_hexdump(fout, t1_out);
    }
    try(buf_eq(t1_out, t1_target));

    // Uses dynamic huffman table
    Buffer t3_target = str_buf(
        "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 "
        "50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80"
    );
    Buffer t3_in = base64_decode(
        mem, str_buf(
                 "H4sIAAAAAAACAw2PCQHAQAzCrERC6UfPv7FNAEkIRFI0w2KOhwIJJSrUaNAio0OPDPLfJFlkk0MuafLIRwUl6kcW1dRQS5k66tFBi076NzY99NKmj35MMGKSKeYPGmYZM8c"
                 "8NlixyRbb7N+7rNljHw4snLhw48H/HePDj4sPGqJXT+gAAAA="
             )
    );
    Buffer t3_out = {};
    try(gzip_read_buffer(mem, t3_in, &t3_out));
    if (0) {
        fmt_hexdump(fout, t3_target);
        fmt_hexdump(fout, t3_in);
        fmt_hexdump(fout, t3_out);
    }
    try(buf_eq(t3_out, t3_target));
    return ok();
}
