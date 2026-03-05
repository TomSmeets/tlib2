// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// deflate.h - DEFLATE decompressor implementation
#pragma once
#include "deflate_huffman.h"
#include "deflate_llcode.h"

// Find index into data containing the prefix sequence of key
static Buffer buf_find_longest_match(Buffer data, size_t start, size_t max_size) {
    size_t match_index = 0;
    size_t match_size = 0;
    if (max_size > start) max_size = start;
    Buffer a = buf_drop(data, start);
    for (size_t i = start - max_size; i < start; ++i) {
        Buffer b = buf_drop(data, i);
        size_t len = buf_match_len(a, b);
        if (len < match_size) continue;
        match_size = len;
        match_index = i;
    }
    return (Buffer){data.data + match_index, match_size};
}

// Compress the input data using LZSS and encode it with the provided huffman code
// The symbol frequencies in the optional argument 'info' are incremented if present
static bool deflate_lzss_encode(Memory *mem, Deflate_Huffman *code, Buffer input, Stream *output_stream, Deflate_Encode_Info *info) {
    // Add values
    size_t match_start = 0;
    size_t match_len = 0;
    for (size_t i = 0; i < input.size; ++i) {
        Buffer match = buf_find_longest_match(input, i, 1 << 15);
        if (1 || match.size <= 3) {
            // Raw symbol
            u16 symbol = input.data[i];
            try(huffman_code_write(code->length, output_stream, symbol));
            if (info) info->length_freq[symbol]++;
            // F(fout, (char)symbol);
        } else {
            // LZSS sequence
            size_t match_ix = input.data - match.data + i;
            // F(fout, "Match: ", match_ix, " ", match.size, EOL);
            i += match.size - 1;
        }
    }

    // End of block marker
    try(huffman_code_write(code->length, output_stream, 256));
    if (info) info->length_freq[256]++;
    return ok();
}

static bool
deflate_lzss_recode(Memory *mem, Deflate_LLCode *llcode, Deflate_Huffman *input_code, Buffer input, Deflate_Huffman *output_code, Stream *output) {
    Stream input_stream = stream_from(input);
    stream_read_bits(&input_stream, 3);

    for (;;) {
        u32 symbol = huffman_code_read(input_code->length, &input_stream);
        huffman_code_write(output_code->length, output, symbol);

        if (symbol == 256) break;
        if (symbol < 256) continue;
        try(symbol < 288);

        // Construct length symbol
        u32 length_symbol = symbol - 256;
        try(length_symbol < array_count(llcode->length_bits));

        // Copy length bits
        u32 length_bits = llcode->length_bits[length_symbol];
        for (u32 i = 0; i < length_bits; ++i) stream_write_bit(output, stream_read_bit(&input_stream));

        // Re-encode distance symbol
        u32 distance_symbol = huffman_code_read(input_code->distance, &input_stream);
        huffman_code_write(output_code->distance, output, distance_symbol);

        // Copy distance bits
        try(distance_symbol < array_count(llcode->distance_bits));
        u32 distance_bits = llcode->distance_bits[distance_symbol];
        for (u32 i = 0; i < distance_bits; ++i) stream_write_bit(output, stream_read_bit(&input_stream));
    }
    return ok();
}
