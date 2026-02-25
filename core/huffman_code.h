// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// huffman.h - Canonical huffman table
#pragma once
#include "fmt.h"
#include "mem.h"
#include "stream.h"

// Canonical huffman code
typedef struct {
    // prefix len -> number of symbols
    u16 counts[15];

    // list of symbols sorted by bit length
    u16 symbols[288];
} Huffman_Code;

static Huffman_Code *huffman_code_from(Memory *mem, u32 count, u8 *symbol_length) {
    Huffman_Code *table = mem_struct(mem, Huffman_Code);
    try(count <= 288);

    // Count number of symbols per bit length
    for (u32 symbol = 0; symbol < count; ++symbol) {
        u8 len = symbol_length[symbol];
        if (len == 0) continue;
        try(len <= 15);
        table->counts[len - 1]++;
    }

    // bit length -> symbol index
    // Will be incremented when symbols are added
    u16 symbol_index[15];
    symbol_index[0] = 0;
    for (u32 len = 1; len < 15; ++len) {
        symbol_index[len] = symbol_index[len - 1] + table->counts[len - 1];
    }

    // Fill in symbol table
    for (u32 symbol = 0; symbol < count; ++symbol) {
        u8 len = symbol_length[symbol];
        if (len == 0) continue;
        table->symbols[symbol_index[len - 1]] = symbol;
        symbol_index[len - 1]++;
    }

    return table;
}

static u32 huffman_code_read(Huffman_Code *table, Stream *stream) {
    // `Maximum deflate bit length is 15

    // First prefix code for the current bit length
    u32 first_code = 0;

    // First symbol index for the current bit length
    u32 first_symbol = 0;

    // Current parsed prefix code
    u32 code = 0;

    // Iterate all bits
    for (u32 i = 0; i < 15; ++i) {
        // Read bit from stream
        u32 bit = stream_read_bit(stream);

        // Prepend bit to the prefix code
        code = (code << 1) | bit;

        // Number of symbols of this bit length
        u8 count = table->counts[i];

        // If the index is outside the range, it has a higher bit length
        if (code < first_code + count) {
            return table->symbols[first_symbol + (code - first_code)];
        }

        // Advance offset to next start of symbols
        first_code = (first_code + count) << 1;
        first_symbol += count;
    }

    // invalid
    return -1;
}

static bool huffman_code_test(void) {
    u8 len[] = {3, 0, 4, 5, 0, 0, 1, 3, 5, 3};
    u32 code[] = {0b100, 0, 0b1110, 0b11110, 0, 0, 0b0, 0b101, 0b11111, 0b110};

    Memory *mem = mem_new();
    Huffman_Code *table = huffman_code_from(mem, array_count(len), len);

    try(table);
    try(table->counts[1 - 1] == 1);
    try(table->counts[3 - 1] == 3);
    try(table->counts[4 - 1] == 1);
    try(table->counts[5 - 1] == 2);

    // Total count == symbol count
    u32 sum = 0;
    for (u32 i = 0; i < array_count(table->counts); ++i) {
        sum += table->counts[i];
    }
    try(sum == 7);

    Stream *stream = stream_new(mem);
    for (u32 sym = 0; sym < array_count(len); ++sym) {
        stream_write_bits_be(stream, len[sym], code[sym]);
        stream_write_bits_be(stream, len[sym], code[sym]);
    }

    stream_seek(stream, 0);

    for (u32 sym = 0; sym < array_count(len); ++sym) {
        if (!len[sym]) continue;

        u32 sym_parse = huffman_code_read(table, stream);
        try(sym_parse != -1);
        try(sym_parse == sym);

        u32 sym_parse2 = huffman_code_read(table, stream);
        try(sym_parse2 != -1);
        try(sym_parse2 == sym);
    }
    try(stream_eof(stream));
    return ok();
}
