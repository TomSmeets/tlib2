// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// parse.h - Recusive decent text parser
#pragma once
#include "mem.h"
#include "type.h"

typedef struct Parse Parse;

struct Parse {
    Memory *mem;
    u64 cursor;
    u64 size;
    u8 *data;
};

static Parse *parse_new(Memory *mem, void *data, u64 size) {
    Parse *parse = mem_struct(mem, Parse);
    parse->cursor = 0;
    parse->size = size;
    parse->data = data;
    return parse;
}

// Check for end of file stream
static bool parse_eof(Parse *parse) {
    return parse->cursor >= parse->size;
}

// Peek next byte
static u8 parse_peek(Parse *parse) {
    if (parse_eof(parse)) return 0;
    return parse->data[parse->cursor];
}

// Go to the next byte
static bool parse_next(Parse *parse) {
    if (parse_eof(parse)) return 0;
    parse->cursor++;
    return true;
}

static void *parse_data(Parse *parse, u32 size) {
    if (parse->cursor + size > parse->size) {
        parse->cursor = parse->size;
        return 0;
    }
    void *ret = parse->data + parse->cursor;
    parse->cursor += size;
    return ret;
}

static u32 parse_u32(Parse *parse) {
    return *(u32 *)parse_data(parse, sizeof(u32));
}

static u16 parse_u16(Parse *parse) {
    return *(u16 *)parse_data(parse, sizeof(u16));
}

static u8 parse_u8(Parse *parse) {
    return *(u8 *)parse_data(parse, sizeof(u8));
}

// Parse unsigned LEB128 integer
static u64 parse_leb128(Parse *parse, bool is_signed) {
    u64 value = 0;
    u32 shift = 0;
    for (;;) {
        u8 byte = parse_u8(parse);
        u8 byte_low = byte & 0x7f;
        u8 byte_high = byte & 0x80;
        value |= (u64)byte_low << shift;
        if (byte_high == 0) break;
        shift += 7;
    }

    if (is_signed) {
        u32 move = 64 - shift;

        // Sign extend
        i64 s_value = value << move;

        // Shift back while sign extending
        s_value >>= move;

        value = s_value;
    }
    return value;
}

// Parse unsigned LEB128 integer
static u64 parse_uleb128(Parse *parse) {
    return parse_leb128(parse, false);
}

// Parse signed LEB128 integer
static i64 parse_ileb128(Parse *parse) {
    return parse_leb128(parse, true);
}

// Parse single line, excluding newline
static Buffer parse_line(Parse *parse) {
    u64 line_start = parse->cursor;
    do {
        u8 chr = parse_peek(parse);
        if (chr == '\n') break;
    } while (parse_next(parse));
    u64 line_end = parse->cursor;
    // Consume newline char (if not eof)
    parse_next(parse);
    return (Buffer){
        parse->data + line_start,
        line_end - line_start,
    };
}

static bool parse_symbol(Parse *parse, char *symbol) {
    u32 len = str_len(symbol);

    // Check fit
    if (parse->cursor + len > parse->size) return false;

    // Check data
    for (u32 i = 0; i < len; ++i) {
        if (parse->data[parse->cursor + i] != symbol[i]) return false;
    }
    parse->cursor += len;
    return true;
}
