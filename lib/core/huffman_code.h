// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// huffman.h - Canonical huffman table
#pragma once
#include "fmt.h"
#include "mem.h"
#include "read.h"
#include "write.h"

// Canonical huffman code
typedef struct {
    // prefix len -> number of symbols
    u16 counts[15];

    // list of symbols sorted by bit length
    u16 symbols[288];

    // Symbols to encoding
    u8 symbol_len[288];
    u16 symbol_code[288];
} Huffman_Code;

static Huffman_Code *huffman_code_from(Memory *mem, u32 count, u8 *symbol_length) {
    Huffman_Code *table = mem_struct(mem, Huffman_Code);

    // Set error flag if too many symbols detected,
    // and try our best to do something anyway
    check_or(count <= 288) count = 288;

    // Count number of symbols per bit length
    for (u32 symbol = 0; symbol < count; ++symbol) {
        u8 len = symbol_length[symbol];

        // Skip non-existing symbols
        if (len == 0) continue;

        // lengths > 15 are invalid
        check_or(len <= 15) len = 15;

        // Count number of symbols of this lengths
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

    // Fill in symbol -> code mapping
    u32 code = 0;
    u32 symbol_ix = 0;
    for (u32 i = 0; i < 15; ++i) {
        for (u32 j = 0; j < table->counts[i]; ++j) {
            u32 symbol = table->symbols[symbol_ix++];
            table->symbol_code[symbol] = code;
            table->symbol_len[symbol] = i + 1;
            code++;
        }
        code <<= 1;
    }
    return table;
}

static u32 huffman_code_read(Huffman_Code *table, Read *read) {
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
        u32 bit = read_bit(read);

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
    check(!"Invalid code");
    return 0;
}

// Write a symbol into a bit stream
static void huffman_code_write(Huffman_Code *table, Write *write, u32 symbol) {
    check_or(symbol < array_count(table->symbol_len)) return;

    u32 len = table->symbol_len[symbol];
    check(len > 0);

    u32 code = table->symbol_code[symbol];
    write_bits_be(write, len, code);
}

static void huffman_code_test(Memory *mem) {
    u8 len[] = {3, 0, 4, 5, 0, 0, 1, 3, 5, 3};
    u32 code[] = {0b100, 0, 0b1110, 0b11110, 0, 0, 0b0, 0b101, 0b11111, 0b110};
    Huffman_Code *table = huffman_code_from(mem, array_count(len), len);

    {
        // Test huffman table creation
        check(table->counts[1 - 1] == 1);
        check(table->counts[3 - 1] == 3);
        check(table->counts[4 - 1] == 1);
        check(table->counts[5 - 1] == 2);

        // Total count == symbol count
        u32 sum = 0;
        for (u32 i = 0; i < array_count(table->counts); ++i) {
            sum += table->counts[i];
        }
        check(sum == 7);
    }

    {
        // Test huffman_code_read
        Write *write = write_new(mem);
        for (u32 sym = 0; sym < array_count(len); ++sym) {
            if (len[sym] == 0) continue;
            write_bits_be(write, len[sym], code[sym]);
            write_bits_be(write, len[sym], code[sym]);
        }

        Read read = read_from(write_get_written(write));
        for (u32 sym = 0; sym < array_count(len); ++sym) {
            if (len[sym] == 0) continue;
            check(huffman_code_read(table, &read) == sym);
            check(huffman_code_read(table, &read) == sym);
        }
        check(read_eof(&read));
    }

    {
        // Test huffman_code_write
        Write *write = write_new(mem);
        for (u32 sym = 0; sym < array_count(len); ++sym) {
            if (len[sym] == 0) continue;
            huffman_code_write(table, write, sym);
            huffman_code_write(table, write, sym);
        }

        Read read = read_from(write_get_written(write));
        for (u32 sym = 0; sym < array_count(len); ++sym) {
            if (len[sym] == 0) continue;
            check(huffman_code_read(table, &read) == sym);
            check(huffman_code_read(table, &read) == sym);
        }
        check(read_eof(&read));
    }
}
