#pragma once
#include "buf.h"
#include "os.h"
#include "str.h"
#include "type.h"

// Compute crc32 (for gzip)
static u32 crc_compute(Buffer buf) {
    u32 poly = 0xedb88320;
    u32 xor = 0xffffffff;

    // Create crc table
    u32 table[256];
    for (u32 n = 0; n < 256; n++) {
        u32 c = n;
        for (u32 k = 0; k < 8; k++) {
            if (c & 1) {
                c = poly ^ (c >> 1);
            } else {
                c = c >> 1;
            }
        }
        table[n] = c;
    }

    // Compute crc
    u32 crc = 0;
    u32 c = crc ^ xor;
    for (size_t i = 0; i < buf.size; i++) {
        c = table[(c ^ ((u8 *)buf.data)[i]) & 0xff] ^ (c >> 8);
    }
    return c ^ xor;
}

static void crc_test(void) {
    assert(crc_compute(str_buf("Hello World!")) == 0x1c291ca3);
    assert(crc_compute(str_buf("1234")) == 0x9be3e0a3);
}
