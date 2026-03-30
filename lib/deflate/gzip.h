// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// gzip.h - GZIP decompressor
#pragma once
#include "base64.h"
#include "crc.h"
#include "deflate.h"
#include "error.h"
#include "fmt.h"
#include "read.h"
#include "mem.h"
#include "stream.h"

static Buffer gzip_read_from(Memory *mem, Read *read) {
    u16 magic = read_u16(read);
    check(magic == 0x8b1f);
    if (error) return buf_null();

    // Compression Method (8 = gzip)
    u8 method = read_u8(read);
    check(method == 0x8);

    u8 flags = read_u8(read);
    bool ftext = (flags >> 0) & 1;
    bool fhcrc = (flags >> 1) & 1;
    bool fextra = (flags >> 2) & 1;
    bool fname = (flags >> 3) & 1;
    bool fcomment = (flags >> 4) & 1;
    check(fextra == 0);

    u32 mtime = read_u32(read);

    // XFL Compression info:
    // 2 -> Best compression
    // 4 -> Fast compression
    u8 xfl = read_u8(read);

    u8 os = read_u8(read);
    while (fname && read_u8(read));
    while (fcomment && read_u8(read));

    if (fhcrc) {
        u16 crc = read_u16(read);
    }

    Buffer result = deflate_read_from(mem, read);

    u32 crc_gzip = read_u32(read);
    u32 size_gzip = read_u32(read);
    u32 crc_real = crc_compute(result);
    check(size_gzip == result.size);
    check(crc_gzip == crc_real);
    check(read_eof(read));
    return result;
}

static Buffer gzip_read(Memory *mem, Buffer input) {
    Read read = read_from(input);
    return gzip_read_from(mem, &read);
}

static Buffer gzip_write(Memory *mem, Buffer input) {
    Write *output = write_new(mem);
    write_u16(output, 0x8b1f); // Magic
    write_u8(output, 0x08);    // method
    write_u8(output, 0);       // flags
    write_u32(output, 0);      // mtime
    write_u8(output, 0);       // xfl
    write_u8(output, 0);       // OS
    write_buffer(output, deflate_write(mem, input));
    write_u32(output, crc_compute(input));
    write_u32(output, input.size);
    u32 crc_comp = crc_compute(input);
    return write_get_written(output);
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
