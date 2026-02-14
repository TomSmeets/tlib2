#pragma once
#include "type.h"

static u32 crc_compute(u8 *buf, size_t len) {
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
    for (u32 n = 0; n < len; n++) {
        c = table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }
    return c ^ xor;
}
