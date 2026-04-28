// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// fmt.h - Text formatter
#pragma once
#include "mem.h"
#include "os.h"
#include "write.h"

typedef struct {
    // Base for printing numbers
    u32 base;
    u32 zero_pad;
    u32 pad;
    bool upper;
    bool base_prefix;
    bool no_color;
    bool need_ansi_reset;
    bool no_eol;

    // Output
    Write *write;
} Fmt;

// Write a null terminated string
static void fmt_s(Fmt *fmt, char *str) {
    write_buffer(fmt->write, str_buf(str));
}

// Write a single character
static void fmt_c(Fmt *fmt, char chr) {
    write_u8(fmt->write, chr);
}

// Write a string slice
static void fmt_buf(Fmt *fmt, Buffer buf) {
    write_buffer(fmt->write, buf);
}

// Reset ansi codes
#define F_Reset (fmt_reset(fmt),"")
static void fmt_reset(Fmt *fmt) {
    if (!fmt->need_ansi_reset) return;
    fmt->need_ansi_reset = 0;
    fmt_s(fmt, "\e[0m");
}

// Write end of line and reset ansi colors
static Buffer fmt_end(Fmt *fmt) {
    fmt_reset(fmt);
    if(!fmt->no_eol) fmt_s(fmt, "\n");
    return write_get_written(fmt->write);
}

// Write ansi color code
// clang-format off
#define F_Ansi(code) (fmt_ansi(fmt, code),"")
#define F_Bold       F_Ansi("\e[1m")
#define F_Faint      F_Ansi("\e[2m")
#define F_Underline  F_Ansi("\e[4m")
#define F_Blink      F_Ansi("\e[5m")
#define F_Invert     F_Ansi("\e[7m")
#define F_Strike     F_Ansi("\e[9m")
#define F_Black      F_Ansi("\e[30m")
#define F_Red        F_Ansi("\e[31m")
#define F_Green      F_Ansi("\e[32m")
#define F_Yellow     F_Ansi("\e[33m")
#define F_Blue       F_Ansi("\e[34m")
#define F_Magenta    F_Ansi("\e[35m")
#define F_Cyan       F_Ansi("\e[36m")
#define F_White      F_Ansi("\e[37m")
#define F_Black_BG   F_Ansi("\e[40m")
#define F_Red_BG     F_Ansi("\e[41m")
#define F_Green_BG   F_Ansi("\e[42m")
#define F_Yellow_BG  F_Ansi("\e[43m")
#define F_Blue_BG    F_Ansi("\e[44m")
#define F_Magenta_BG F_Ansi("\e[45m")
#define F_Cyan_BG    F_Ansi("\e[46m")
#define F_White_BG   F_Ansi("\e[47m")
// clang-format on
static void fmt_ansi(Fmt *fmt, char *code) {
    if (fmt->no_color) return;
    fmt->need_ansi_reset = 1;
    fmt_s(fmt, code);
}

#define F_Pad(pad) (fmt_pad(fmt, pad),"")
static void fmt_pad(Fmt *fmt, u32 pad) {
    fmt->pad = pad;
}

#define F_ZeroPad(pad) (fmt_zero_pad(fmt, pad),"")
static void fmt_zero_pad(Fmt *fmt, u32 pad) {
    fmt->zero_pad = pad;
}

#define F_Base(base) (fmt_base(fmt, base),"")
#define F_Hex F_Base(16)
#define F_Bin F_Base(2)
static void fmt_base(Fmt *fmt, u32 base) {
    fmt->base = base;
}

#define F_NoColor (fmt_no_color(fmt),"")
static void fmt_no_color(Fmt *fmt) {
    fmt->no_color = 1;
}

#define F_NoEOL (fmt->no_eol = 1,"")

// Format any integer
static void fmt_int(Fmt *fmt, bool is_signed, u64 value) {
    // Configuration
    u32 base = fmt->base ?: 10;

    // Reversed digits
    u32 count = 0;
    char buffer[1024];
    do {
        u32 rem = value % base;
        u8 digit;
        if (rem < 10) {
            digit = rem + '0';
        } else {
            digit = rem - 10 + (fmt->upper ? 'A' : 'a');
        }
        buffer[count++] = digit;
        value /= base;
    } while (value > 0);

    while (count < fmt->zero_pad) {
        buffer[count++] = '0';
    }

    if (fmt->base_prefix) {
        if (base == 16) {
            buffer[count++] = 'x';
            buffer[count++] = '0';
        }
        if (base == 8) {
            buffer[count++] = '0';
        }
        if (base == 2) {
            buffer[count++] = 'b';
            buffer[count++] = '0';
        }
    }

    // Append sign char
    if (is_signed && (i64)value < 0) {
        buffer[count++] = '-';
        value = -(i64)value;
    }

    // Whitespace padding
    while (count < fmt->pad) {
        buffer[count++] = ' ';
    }

    // Reverse buffer
    ptr_reverse(buffer, count);
    fmt_buf(fmt, buf_from(buffer, count));
}

// Format unsigned
static void fmt_u64(Fmt *fmt, u64 value) {
    fmt_int(fmt, 0, value);
}

// Format signed
static void fmt_i64(Fmt *fmt, i64 value) {
    fmt_int(fmt, 1, value);
}

static void fmt_color_fg(Fmt *fmt, u8 r, u8 g, u8 b) {
    if (fmt->no_color) return;
    fmt->need_ansi_reset = 1;
    fmt_s(fmt, "\e[38;2;");
    fmt_u64(fmt, r);
    fmt_s(fmt, ";");
    fmt_u64(fmt, g);
    fmt_s(fmt, ";");
    fmt_u64(fmt, b);
    fmt_s(fmt, "m");
}

static void fmt_color_bg(Fmt *fmt, u8 r, u8 g, u8 b) {
    if (fmt->no_color) return;
    fmt->need_ansi_reset = 1;
    fmt_s(fmt, "\e[48;2;");
    fmt_u64(fmt, r);
    fmt_s(fmt, ";");
    fmt_u64(fmt, g);
    fmt_s(fmt, ";");
    fmt_u64(fmt, b);
    fmt_s(fmt, "m");
}

// clang-format off
#define fmt_generic(x)           \
    _Generic((x),                \
        char *: fmt_s,   \
        i64:    fmt_i64, \
        u64:    fmt_u64, \
        i32:    fmt_i64, \
        u32:    fmt_u64, \
        i16:    fmt_i64, \
        u16:    fmt_u64, \
         i8:    fmt_i64, \
         u8:    fmt_u64, \
        char:   fmt_c,   \
        Buffer: fmt_buf  \
    )(fmt, x)
// clang-format on

// Format to writer
#define write_fmt(W, ...) \
    ({ \
        Fmt fmt1 = {.write = (W) } \
        Fmt fmt = &fmt1; \
        REPEAT0(fmt_generic, __VA_ARGS__); \
    })

// Format to string
#define fstr(mem, ...) \
    ({ \
        Write write = {.mem = mem}; \
        Fmt _fmt = {.write = &write}; \
        Fmt *fmt = &_fmt; \
        REPEAT0(fmt_generic, __VA_ARGS__); \
        write_get_written(fmt->write); \
    })

// Format and write to file directly
#define fprint(OUT, ...) \
    ({ \
        Memory *mem = mem_new(); \
        Write write = {.mem = mem}; \
        Fmt fmt1 = {.write = &write}; \
        Fmt *fmt = &fmt1; \
        REPEAT0(fmt_generic, __VA_ARGS__); \
        Buffer result = fmt_end(fmt); \
        os_write(OUT, result); \
        mem_free(mem); \
    })

#define print(...) fprint(os_stdout(), __VA_ARGS__)
#define debug(x) fprint(os_stderr(), F_Faint, __FILE__ ":" TO_STRING(__LINE__) ": ", F_Reset, #x, " = ", x)

// TODO: no more 'file' in fmt, just format string
static void test_fmt(void) {
    Memory *mem = mem_new();
    Buffer str1 = fstr(mem, F_Red, "Hello", F_Reset);
    Buffer str2 = fstr(mem, F_Blue, "World", F_Reset);
    print("Hello ", F_Yellow, F_Base(16), F_ZeroPad(16), "0x", 1234, F_Reset, " World! === ", str1, str2);
    for (u32 i = 0; i < 10; ++i) {
        debug(i);
        print("b=", F_Red, F_Bin,  i, F_Reset," x=", F_Blue, F_Hex, i);
    }
    Fmt fmt = {.write = write_new(mem), .no_eol = 1};
    for (u32 r = 0; r < 256; r += 8) {
        for (u32 g = 0; g < 256; g += 8) {
            for (u32 b = 0; b < 256; b += 8) {
               fmt_color_bg(&fmt, r, g, b);
               fmt_s(&fmt, "  ");
            }
            fmt_s(&fmt, "\n");
        }
    }
    os_write(os_stdout(), fmt_end(&fmt));
    mem_free(mem);
}
