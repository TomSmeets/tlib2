// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// base64.h - Readable Base64 encoding and decoding
#pragma once
#include "mem.h"
#include "error.h"
#include "type.h"

// Convert a byte to the correct base64 char
// - Considers only the bottom 6 bits
static u8 base64_from_bits(u8 c) {
    // Only consider lower 6  bits
    c &= 63;

    if (c < 26) return c - 0 + 'A';
    if (c < 52) return c - 26 + 'a';
    if (c < 62) return c - 52 + '0';
    if (c == 62) return '+';
    if (c == 63) return '/';

    // invalid
    return 0;
}

// Convert a base64 char to bits
// returns a 6 bit value
static u8 base64_to_bits(u8 c) {
    if (c >= 'A' && c <= 'Z') return c - 'A' + 0;
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;

    // invalid
    return 0;
}

static Buffer base64_encode(Memory *mem, Buffer input) {
    // Converts: 3 * 8 bit -> 4 * 6 bit
    //
    // Every 3 bytes are converted to 4 characters
    // Round up, the rest is padded with'='
    //
    //          <-----  Chunk  ------>
    //
    // in:    |---a---|---b---|---c---|
    // out:   |--X--|--Y--|--Z--|--W--|
    size_t input_count = input.size;
    u8 *input_data = input.data;

    // Calculate number of output bytes
    // 1. count data bits (8 bits)
    // 2. count base64 chars rounded up (6 bits)
    size_t output_count = (input_count * 8 + 5) / 6;

    // Calculate buffer size with padding
    size_t chunk_count = (input_count + 2) / 3;
    size_t buffer_size = chunk_count * 4;

    // Allocate size for entire buffer + zero terminator
    u8 *output_data = mem_array(mem, u8, buffer_size + 1);
    output_data[buffer_size] = 0;

    for (size_t i = 0, j = 0; i < input_count && j < output_count;) {
        u32 chunk_value = 0;
        if (i < input_count) chunk_value |= input_data[i++] << (8 * 2);
        if (i < input_count) chunk_value |= input_data[i++] << (8 * 1);
        if (i < input_count) chunk_value |= input_data[i++] << (8 * 0);
        if (j < output_count)
            output_data[j++] = base64_from_bits(chunk_value >> (6 * 3));
        else
            output_data[j++] = '=';
        if (j < output_count)
            output_data[j++] = base64_from_bits(chunk_value >> (6 * 2));
        else
            output_data[j++] = '=';
        if (j < output_count)
            output_data[j++] = base64_from_bits(chunk_value >> (6 * 1));
        else
            output_data[j++] = '=';
        if (j < output_count)
            output_data[j++] = base64_from_bits(chunk_value >> (6 * 0));
        else
            output_data[j++] = '=';
        assert(j <= buffer_size);
    }
    return (Buffer){output_data, buffer_size};
}

// Decode base64 data
// Trailing '=' are optional
// Returns zero terminated data
static Buffer base64_decode(Memory *mem, Buffer input) {
    size_t input_count = input.size;
    u8 *input_data = input.data;

    // Trim trailing '='
    while (input_count > 0 && input_data[input_count - 1] == '=') input_count--;

    // Calculate number of output bytes
    // 1. count base64 bits (6 bits)
    // 2. count output bytes rounded down (8 bits)
    size_t output_count = input_count * 6 / 8;

    // Allocate size for entire buffer + zero terminator
    u8 *output_data = mem_array(mem, u8, output_count + 1);
    output_data[output_count] = 0;

    for (size_t i = 0, j = 0; i < input_count && j < output_count;) {
        u32 chunk_value = 0;
        if (i < input_count) chunk_value |= base64_to_bits(input_data[i++]) << (6 * 3);
        if (i < input_count) chunk_value |= base64_to_bits(input_data[i++]) << (6 * 2);
        if (i < input_count) chunk_value |= base64_to_bits(input_data[i++]) << (6 * 1);
        if (i < input_count) chunk_value |= base64_to_bits(input_data[i++]) << (6 * 0);
        if (j < output_count) output_data[j++] = chunk_value >> (8 * 2);
        if (j < output_count) output_data[j++] = chunk_value >> (8 * 1);
        if (j < output_count) output_data[j++] = chunk_value >> (8 * 0);
    }
    return (Buffer){output_data, output_count};
}

// ==== Testing ====
static bool base64_test_encode(Buffer input, char *expect) {
    bool ok = true;
    Memory *mem = mem_new();

    char *output = base64_encode(mem, input).data;
    ok = str_eq(output, expect);

    Buffer reverse = base64_decode(mem, str_buf(output));
    ok = ok && buf_eq(input, reverse);

    mem_free(mem);
    return ok;
}

static bool base64_test(void) {
    try(base64_test_encode(str_buf("Hello World!"), "SGVsbG8gV29ybGQh"));
    try(base64_test_encode(str_buf(""), ""));
    try(base64_test_encode(str_buf("a"), "YQ=="));
    try(base64_test_encode(str_buf("aa"), "YWE="));
    try(base64_test_encode(str_buf("aaa"), "YWFh"));
    try(base64_test_encode(str_buf("aaaa"), "YWFhYQ=="));
    try(base64_test_encode(BUFFER(u8, 0, 0, 0, 0), "AAAAAA=="));
    try(base64_test_encode(BUFFER(u8, 1, 2, 3, 4, 5, 6), "AQIDBAUG"));
    return ok();
}
