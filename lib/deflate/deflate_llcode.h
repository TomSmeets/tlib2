// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// deflate.h - DEFLATE decompressor implementation
#pragma once
#include "mem.h"
#include "stream.h"
#include "type.h"

// Symbol to actual length / distance encoding
// Each symbol starts at some offset is followed
// by a number of bits
typedef struct {
    // To calculate the length value:
    // - length_value = length_offset[sym] + read(length_bits[sym])
    u8 length_bits[29];
    u32 length_offset[29];

    // To calculate the distance value:
    // - distance_value = distance_offset[sym] + read(distance_bits[sym])
    u8 distance_bits[30];
    u32 distance_offset[30];
} Deflate_LLCode;

// Create the fixed llcode table
static Deflate_LLCode *deflate_llcode_new(Memory *mem) {
    Deflate_LLCode *code = mem_struct(mem, Deflate_LLCode);

    // Length values
    for (u32 i = 4; i < 28; ++i) {
        code->length_bits[i] = (i - 4) / 4;
    }

    // Distance values
    for (u32 i = 2; i < 30; ++i) {
        code->distance_bits[i] = (i - 2) / 2;
    }

    // Length offset
    code->length_offset[0] = 3;
    code->length_offset[28] = 258;
    for (u32 i = 1; i < 28; ++i) {
        u32 start = code->length_offset[i - 1];
        u32 bits = code->length_bits[i - 1];
        code->length_offset[i] = start + (1 << bits);
    }

    // Distance offset
    code->distance_offset[0] = 1;
    for (u32 i = 1; i < 30; ++i) {
        u32 start = code->distance_offset[i - 1];
        u32 bits = code->distance_bits[i - 1];
        code->distance_offset[i] = start + (1 << bits);
    }
    return code;
}

// Read construct an absolute length by reading the extra bits from the input stream based on the length_symbol
static u32 deflate_llcode_length_read(Deflate_LLCode *code, Stream *input, u16 length_symbol) {
    assert(length_symbol < 29);
    u32 bits = code->length_bits[length_symbol];
    u32 offset = code->length_offset[length_symbol];
    u32 data = stream_read_bits(input, bits);
    return offset + data;
}

// Read construct an absolute distance by reading the extra bits from the input stream based on the distance_symbol
static u32 deflate_llcode_distance_read(Deflate_LLCode *code, Stream *input, u16 distance_symbol) {
    assert(distance_symbol < 30);
    u32 bits = code->distance_bits[distance_symbol];
    u32 offset = code->distance_offset[distance_symbol];
    u32 data = stream_read_bits(input, bits);
    return offset + data;
}
