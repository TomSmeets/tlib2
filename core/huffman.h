#pragma once
#include "mem.h"
#include "fmt.h"
#include "stream.h"


typedef struct {
    // symbol count
    u32 cap;
    u32 count;

    // Symbol -> bits
    u8 *len;

    // Symbol -> code
    u32 *code;
} Huffman;

static Huffman *huffman_new(Memory *mem, u32 max_count) {
    Huffman *cc = mem_struct(mem, Huffman);
    cc->cap = max_count;
    cc->len  = mem_array(mem, u8, max_count);
    cc->code = mem_array(mem, u32, max_count);
    return cc;
}

static void huffman_add(Huffman *huffman, u8 bit_count) {
    assert(huffman->count < huffman->cap);
    u32 sym = huffman->count++;
    huffman->len[sym] = bit_count;
    huffman->code[sym] = 0;
}

static void huffman_build(Huffman *huffman) {
    u32 min = huffman->len[0];
    u32 max = huffman->len[0];
    for(u32 i = 0; i < huffman->count; ++i) {
        if(huffman->len[i] < min) min = huffman->len[i];
        if(huffman->len[i] > max) max = huffman->len[i];
    }
    u32 code = 0;
    u32 prev_bits = 0;
    for(u32 bits = min; bits <= max; ++bits) {
        for(u32 sym = 0 ; sym < huffman->count; ++sym) {
            u32 sym_bits = huffman->len[sym];
            if(bits != sym_bits) continue;
            if(code) code <<= bits - prev_bits;
            huffman->code[sym] = code;
            code += 1;
            prev_bits = bits;
        }
    }
}

typedef struct {
    bool valid;
    u32 code;
    u32 symbol;
    u8 len;
} Huffman_Result;

static Huffman_Result huffman_get_symbol(Huffman *huffman, u8 len, u32 code) {
    Huffman_Result res = {};
    for (u32 sym = 0; sym < huffman->count; ++sym) {
        if (huffman->len[sym] != len) continue;
        if (huffman->code[sym] != code) continue;
        res.valid = 1;
        res.code = code;
        res.symbol = sym;
        res.len = len;
        return res;
    }
    return res;
}

static Huffman_Result huffman_get_code(Huffman *huffman, u32 symbol) {
    Huffman_Result res = {};
    if(symbol >= huffman->count) return res;
    res.valid = 1;
    res.code = huffman->code[symbol];
    res.len = huffman->len[symbol];
    res.symbol = symbol;
    return res;
}

static Huffman_Result huffman_read(Huffman *h, Stream *input) {
    Huffman_Result res = {};
    u32 len  = 0;
    u32 code = 0;
    while (1) {
        code = (code << 1) | stream_read_bit(input);
        len++;

        res = huffman_get_symbol(h, len, code);
        if(res.valid) return res;
        if(len == 32) break;
    }
    return res;
}

static void huffman_test(void) {
    u32 len[] = {3, 4, 5, 1, 3, 5, 3};
    u32 code[] = {0b100, 0b1110, 0b11110, 0b0, 0b101, 0b11111, 0b110};

    Memory *mem = mem_new();
    Huffman *h = huffman_new(mem, array_count(len));
    for(u32 sym = 0; sym < array_count(len); ++sym) {
        huffman_add(h, len[sym]);
    }
    huffman_build(h);
    for(u32 sym = 0; sym < array_count(len); ++sym) {
        Huffman_Result res = huffman_get_code(h, sym);
        assert(res.valid);
        assert(res.code  == code[sym]);
        assert(res.len == len[sym]);
        assert(res.symbol  == sym);
    }
    for(u32 sym = 0; sym < array_count(len); ++sym) {
        Huffman_Result res = huffman_get_symbol(h, len[sym], code[sym]);
        assert(res.valid);
        assert(res.code  == code[sym]);
        assert(res.len == len[sym]);
        assert(res.symbol  == sym);
    }
    for(u32 sym = 0; sym < array_count(len); ++sym) {
        Huffman_Result res = huffman_get_symbol(h, len[sym] - 1, code[sym]);
        assert(!res.valid);
    }
    for(u32 sym = 0 ; sym < h->count; ++sym) {
        fmt_s(fout, "Code: ");
        fmt_u_ex(fout, sym, 10, ' ', 4);
        fmt_s(fout, " ");
        fmt_u_ex(fout, h->code[sym], 2, '0', h->len[sym]);
        fmt_s(fout, "\n");
    }

    Stream *s = stream_new(mem);
    for(u32 sym = 0 ; sym < h->count; ++sym) {
        stream_write_bits_be(s, len[sym], code[sym]);
        stream_write_bits_be(s, len[sym], code[sym]);
    }
    stream_seek(s, 0);
    for (u32 sym = 0; sym < h->count; ++sym) {
        Huffman_Result res = huffman_read(h, s);
        assert(res.valid);
        assert(res.symbol == sym);
        assert(res.code == code[sym]);
        assert(res.len == len[sym]);

        res = huffman_read(h, s);
        assert(res.valid);
        assert(res.symbol == sym);
        assert(res.code == code[sym]);
        assert(res.len == len[sym]);
    }
    assert(stream_eof(s));
}
