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

static Buffer gzip_read(Memory *mem, Buffer input_buf) {
    Stream input = stream_from_buffer(input_buf);
    u16 magic = stream_read_u16(&input);
    check(magic == 0x8b1f);
    if (error) return buf_null();

    // Compression Method (8 = gzip)
    u8 method = stream_read_u8(&input);
    check(method == 0x8);

    u8 flags = stream_read_u8(&input);
    bool ftext = (flags >> 0) & 1;
    bool fhcrc = (flags >> 1) & 1;
    bool fextra = (flags >> 2) & 1;
    bool fname = (flags >> 3) & 1;
    bool fcomment = (flags >> 4) & 1;
    check(fextra == 0);

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
    Buffer ret = deflate_read(mem, deflate);

    u32 crc_gzip = stream_read_u32(&input);
    u32 size_gzip = stream_read_u32(&input);
    u32 crc_real = crc_compute(ret);
    check(size_gzip == ret.size);
    check(crc_gzip == crc_real);
    check(stream_eof(&input));
    return ret;
}

static Buffer gzip_write(Memory *mem, Buffer input) {
    Stream *output = stream_new(mem);
    stream_write_u16(output, 0x8b1f); // Magic
    stream_write_u8(output, 0x08);    // method
    stream_write_u8(output, 0);       // flags
    stream_write_u32(output, 0);      // mtime
    stream_write_u8(output, 0);       // xfl
    stream_write_u8(output, 0);       // OS
    Buffer deflated_buffer = deflate_write(mem, input);
    stream_write_buffer(output, deflated_buffer);
    stream_write_u32(output, crc_compute(input));
    stream_write_u32(output, input.size);
    u32 crc_comp = crc_compute(input);
    return stream_as_buffer(output);
}

// Run a deflate/inflate testcase with a given input
static void gzip_test_buf(Memory *mem, Buffer input) {
    Buffer compressed = gzip_write(mem, input);
    if (error) return;

    Buffer decompressed = gzip_read(mem, compressed);
    if (error) return;

    check(buf_eq(decompressed, input));
}

static void gzip_test(Memory *mem) {
    {
        Buffer target = str_buf("hello hello world hello hello\n");
        Buffer input = base64_decode(mem, str_buf("H4sIAAAAAAAAA8tIzcnJV8gAk+X5RTkpUDaY5AIAmdZcBR4AAAA="));
        Buffer output = gzip_read(mem, input);
        check(buf_eq(target, output));
    }

    if (!error) {
        Buffer target = deflate_read(mem, str_buf("hello hello\n"));
        // Make sure there was an error
        check(error_pop());
    }

    {
        Buffer target = str_buf("hello hello world hello hello\n");
        Buffer input = gzip_write(mem, target);
        Buffer output = gzip_read(mem, input);
        check(buf_eq(target, output));
    }

    {
        // Uses fixed huffman table
        Buffer target = base64_decode(mem, str_buf("GnSwX91w7Z9EqpaZeyPCIQ=="));
        Buffer input = base64_decode(mem, str_buf("H4sICHPOkWkAA2RhdGEAkyrZEH+34O18l1XTZlYrH1IEAFve5PUQAAAA"));
        Buffer output = gzip_read(mem, input);
        check(buf_eq(output, target));
    }

    {
        // Uses dynamic huffman table
        Buffer target = str_buf(
            "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 "
            "48 49 "
            "50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80"
        );
        Buffer input = base64_decode(
            mem,
            str_buf(
                "H4sIAAAAAAACAw2PCQHAQAzCrERC6UfPv7FNAEkIRFI0w2KOhwIJJSrUaNAio0OPDPLfJFlkk0MuafLIRwUl6kcW1dRQS5k66tFBi076NzY99NKmj35MMGKSKeYPGmYZM8c"
                "8NlixyRbb7N+7rNljHw4snLhw48H/HePDj4sPGqJXT+gAAAA="
            )
        );
        Buffer output = gzip_read(mem, input);
        check(buf_eq(output, target));
    }
}
