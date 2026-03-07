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

static bool gzip_read(Memory *mem, Buffer input_buf, Buffer *output_buf) {
    Stream input = stream_from(input_buf);
    u16 magic = stream_read_u16(&input);
    try(magic == 0x8b1f, "Invalid magic");

    // Compression Method (8 = gzip)
    u8 method = stream_read_u8(&input);
    try(method == 0x8, "Invalid compression method, expecting deflate");

    u8 flags = stream_read_u8(&input);
    bool ftext = (flags >> 0) & 1;
    bool fhcrc = (flags >> 1) & 1;
    bool fextra = (flags >> 2) & 1;
    bool fname = (flags >> 3) & 1;
    bool fcomment = (flags >> 4) & 1;
    try(fextra == 0, "Unsupported gzip flag");

    u32 mtime = stream_read_u32(&input);

    // XFL Compression info:
    // 2 -> Best compression
    // 4 -> Fast compression
    u8 xfl = stream_read_u8(&input);

    u8 os = stream_read_u8(&input);
    while (fname && stream_read_u8(&input));
    while (fcomment && stream_read_u8(&input));

    if (fhcrc) {
        u16 crc = stream_read_u16(&input);
    }

    Buffer deflate = stream_read_buffer(&input, stream_remaining(&input) - 8);
    try(deflate_read(mem, deflate, output_buf));

    u32 crc_gzip = stream_read_u32(&input);
    u32 size_gzip = stream_read_u32(&input);
    u32 crc_real = crc_compute(*output_buf);
    try(size_gzip == output_buf->size);
    try(crc_gzip == crc_real);
    try(stream_eof(&input));
    return ok();
}

static bool gzip_write(Memory *mem, Buffer input, Buffer *output_buf) {
    Stream *output = stream_new(mem);
    stream_write_u16(output, 0x8b1f); // Magic
    stream_write_u8(output, 0x08);    // method
    stream_write_u8(output, 0);       // flags
    stream_write_u32(output, 0);      // mtime
    stream_write_u8(output, 0);       // xfl
    stream_write_u8(output, 0);       // OS
    Buffer deflated_buffer = {};
    try(deflate_write(mem, input, &deflated_buffer));
    try(stream_write_buffer(output, deflated_buffer));
    stream_write_u32(output, crc_compute(input));
    stream_write_u32(output, input.size);
    u32 crc_comp = crc_compute(input);
    *output_buf = stream_to_buffer(output);
    return ok();
}

static bool gzip_test(void) {
    Memory *mem = mem_new();
    {
        Buffer target = str_buf("hello hello world hello hello\n");
        print("Target:\n", target);

        Buffer input = base64_decode(mem, str_buf("H4sIAAAAAAAAA8tIzcnJV8gAk+X5RTkpUDaY5AIAmdZcBR4AAAA="));
        print("Input:\n", input);

        Buffer output = {};
        try(gzip_read(mem, input, &output));
        print("Output:\n", output);

        try(buf_eq(target, output));
    }

    {
        Buffer target = str_buf("hello hello world hello hello\n");
        print("Target:\n", target);

        Buffer input = {};
        try(gzip_write(mem, target, &input));
        print("Input:\n", input);

        Buffer output = {};
        try(gzip_read(mem, input, &output));
        print("Output:\n", output);

        try(buf_eq(target, output));
    }

    // Uses fixed huffman table
    Buffer t1_target = base64_decode(mem, str_buf("GnSwX91w7Z9EqpaZeyPCIQ=="));
    Buffer t1_in = base64_decode(mem, str_buf("H4sICHPOkWkAA2RhdGEAkyrZEH+34O18l1XTZlYrH1IEAFve5PUQAAAA"));
    Buffer t1_out = {};
    try(gzip_read(mem, t1_in, &t1_out));
    if (0) {
        print(t1_target);
        print(t1_out);
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
    try(gzip_read(mem, t3_in, &t3_out));
    if (0) {
        print(t3_target);
        print(t3_in);
        print(t3_out);
    }
    try(buf_eq(t3_out, t3_target));
    return ok();
}
