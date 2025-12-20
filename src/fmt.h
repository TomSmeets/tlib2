// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// fmt.h - Text formatter
#pragma once
#include "mem.h"
#include "str.h"
#include "type.h"

typedef struct Fmt Fmt;
struct Fmt {
    Memory *mem;
    File *file;
    bool flush_on_newline;
    u32 size;
    u32 used;
    u8 *data;
};

// Standard formatters
static Fmt *fmt_stdout(void) {
    static u8 buffer[1024];
    static Fmt fmt = {
        .size = sizeof(buffer),
        .data = buffer,
        .flush_on_newline = 1,
    };
    if (!fmt.file) fmt.file = os_stdout();
    return &fmt;
}

static Fmt *fmt_stderr(void) {
    static u8 buffer[1024];
    static Fmt fmt = {
        .size = sizeof(buffer),
        .data = buffer,
        .flush_on_newline = 1,
    };
    if (!fmt.file) fmt.file = os_stderr();
    return &fmt;
}

#define fout (fmt_stdout())
#define ferr (fmt_stderr())

// Create a new formatter that allocates memory
static Fmt *fmt_new(Memory *mem) {
    Fmt *fmt = mem_struct(mem, Fmt);
    fmt->mem = mem;
    return fmt;
}

// Create a fromatter that writes to a buffer
static Fmt fmt_from(u8 *data, u32 size) {
    Fmt fmt = {};
    fmt.size = size;
    fmt.data = data;
    fmt.used = 0;
    return fmt;
}

// Write all buffered data to file
static void fmt_flush(Fmt *fmt) {
    if (!fmt->file) return;
    u32 written = 0;
    assert(os_write(fmt->file, fmt->data, fmt->used, &written));
    assert(written == fmt->used);
    fmt->used = 0;
}

// Try to grow formatter to fit 'size' new data
// Returns true if the new size would fit
static bool fmt_grow(Fmt *fmt, u32 size) {
    // Check if the buffer needs to grow
    if (fmt->used + size <= fmt->size) return true;

    // Try to flush to the file
    fmt_flush(fmt);

    // Check again
    if (fmt->used + size <= fmt->size) return true;

    // Cannot allocate if we have no memory
    if (!fmt->mem) return false;

    // Calculated new size (a power of two)
    u32 new_size = fmt->size * 2;
    if (new_size < 64) new_size = 64;
    while (fmt->used + size > new_size) new_size *= 2;

    // Allocate data
    u8 *new_data = mem_array(fmt->mem, u8, new_size);
    std_memcpy(new_data, fmt->data, fmt->used);
    fmt->size = new_size;
    fmt->data = new_data;
    return true;
}

static void fmt_c(Fmt *fmt, u8 c) {
    if (!fmt_grow(fmt, 1)) return;
    fmt->data[fmt->used++] = c;
    if (fmt->flush_on_newline && c == '\n') fmt_flush(fmt);
}

static void fmt_buf(Fmt *fmt, void *data, u32 size) {
    for (u32 i = 0; i < size; ++i) {
        fmt_c(fmt, ((u8 *)data)[i]);
    }
}

static void fmt_s(Fmt *fmt, char *str) {
    if (!str) return;
    u32 len = str_len(str);
    fmt_buf(fmt, str, len);
}

static char *fmt_end(Fmt *fmt) {
    if (fmt->file) {
        fmt_flush(fmt);
        return 0;
    }

    // Zero terminate
    fmt_c(fmt, 0);
    return (char *)fmt->data;
}

static void fmt_ss(Fmt *fmt, char *arg1, char *arg2, char *arg3) {
    fmt_s(fmt, arg1);
    fmt_s(fmt, arg2);
    fmt_s(fmt, arg3);
}

static void fmt_u_ex(Fmt *fmt, u64 value, u32 base, u8 pad_char, u32 pad) {
    u32 digit_count = 0;
    u8 digit_list[64];
    assert(base >= 2 && base <= 16);
    if (pad > array_count(digit_list)) pad = array_count(digit_list);
    do {
        u8 digit = value % base;
        u8 chr = digit < 10 ? '0' + digit : 'A' + (digit - 10);
        digit_list[digit_count++] = chr;
        value /= base;
    } while (value > 0);

    while (digit_count < pad) {
        digit_list[digit_count++] = pad_char;
    }

    for (u32 i = 0; i < digit_count; ++i) {
        fmt_c(fmt, digit_list[digit_count - i - 1]);
    }
}

static void fmt_u(Fmt *fmt, u64 value) {
    fmt_u_ex(fmt, value, 10, 0, 0);
}

static void fmt_i(Fmt *fmt, i64 value) {
    if (value < 0) {
        fmt_s(fmt, "-");
        value = -value;
    }
    fmt_u_ex(fmt, value, 10, 0, 0);
}

static void fmt_x(Fmt *fmt, u64 value) {
    fmt_s(fmt, "0x");
    fmt_u_ex(fmt, value, 16, 0, 0);
}

static void fmt_sp(Fmt *fmt, char *arg1, void *arg2, char *arg3) {
    fmt_s(fmt, arg1);
    // fmt_p(fmt, arg2);
    fmt_s(fmt, arg3);
}

static void fmt_su(Fmt *fmt, char *arg1, u64 arg2, char *arg3) {
    fmt_s(fmt, arg1);
    fmt_u(fmt, arg2);
    fmt_s(fmt, arg3);
}
static void fmt_si(Fmt *fmt, char *arg1, i64 arg2, char *arg3) {
    fmt_s(fmt, arg1);
    fmt_i(fmt, arg2);
    fmt_s(fmt, arg3);
}

static void fmt_sx(Fmt *fmt, char *arg1, u64 arg2, char *arg3) {
    fmt_s(fmt, arg1);
    fmt_x(fmt, arg2);
    fmt_s(fmt, arg3);
}

static void fmt_pad_line(Fmt *fmt, u32 line_len, u8 pad_char) {
    u32 line_start = fmt->used;
    u32 line_end = fmt->used;

    for (;;) {
        if (line_start == 0) break;
        if (fmt->data[line_start - 1] == '\n') {
            break;
        }
        line_start--;
    }

    for (u32 i = line_end; i < line_start + line_len; ++i) {
        fmt_c(fmt, pad_char);
    }
}

static void test_fmt(void) {
    Memory *mem = mem_new();
    Fmt *fmt = fmt_new(mem);
    fmt_s(fmt, "Hello");
    fmt_s(fmt, " ");
    fmt_s(fmt, "World: ");
    fmt_u(fmt, 1234);
    fmt_s(fmt, "\n");

    char *output = fmt_end(fmt);
    assert(str_eq(output, "Hello World: 1234\n"));
    mem_free(mem);
}
