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

static bool parse_eof(Parse *parse) {
    return parse->cursor >= parse->size;
}

static u8 parse_peek(Parse *parse) {
    if (parse_eof(parse)) return 0;
    return parse->data[parse->cursor];
}

static bool parse_next(Parse *parse) {
    if (parse_eof(parse)) return 0;
    parse->cursor++;
    return true;
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
