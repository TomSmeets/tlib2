// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// fmt.h - Text formatter
#pragma once
#include "error.h"
#include "mem.h"
#include "ptr.h"
#include "str.h"
#include "type.h"
#include "write.h"

// Formatting options
typedef struct {
    u32 base;
    u32 pad;
    u8 pad_chr;
} Fmt_Options;

typedef struct {
    // Memory used to grow the buffer (Optional)
    Memory *mem;

    // Output file or stream (Optional)
    File *file;

    // Flush on newline, otherwise flush when buffer becomes full
    bool flush_on_newline;

    // Buffer storage
    size_t size;
    size_t used;
    u8 *data;

    Fmt_Options opt;
} Fmt;

// Standard formatters
static Fmt *fmt_stdout(void) {
    // TODO: get rid of this
    static thread_local u8 buffer[1024];
    static thread_local Fmt fmt = {
        .size = sizeof(buffer),
        .flush_on_newline = 1,
    };
    if (!fmt.file) {
        fmt.data = buffer;
        fmt.file = os_stdout();
    }
    return &fmt;
}

static Fmt *fmt_stderr(void) {
    static thread_local u8 buffer[1024];
    static thread_local Fmt fmt = {
        .size = sizeof(buffer),
        .flush_on_newline = 1,
    };
    if (!fmt.file) {
        fmt.data = buffer;
        fmt.file = os_stderr();
    }
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

// Create a formatter that writes to a buffer
static Fmt fmt_from(u8 *data, size_t size) {
    Fmt fmt = {};
    fmt.size = size;
    fmt.data = data;
    fmt.used = 0;
    return fmt;
}

// Write all buffered data to file
static void fmt_flush(Fmt *fmt) {
    if (!fmt->file) return;
    Buffer data = buf_from(fmt->data, fmt->used);
    size_t written = os_write(fmt->file, data);
    check(written == data.size);
    fmt->used = 0;
}

// Try to grow formatter to fit 'size' new data
// Returns true if the new size would fit
static bool fmt_grow(Fmt *fmt, size_t size) {
    // Check if the buffer needs to grow
    if (fmt->used + size <= fmt->size) return true;

    // Try to flush to the file
    fmt_flush(fmt);

    // Check again
    if (fmt->used + size <= fmt->size) return true;

    // Cannot allocate if we have no memory
    if (!fmt->mem) return false;

    // Calculated new size (a power of two)
    size_t new_size = fmt->size * 2;
    if (new_size < 64) new_size = 64;
    while (fmt->used + size > new_size) new_size *= 2;

    // Allocate data
    u8 *new_data = mem_array(fmt->mem, u8, new_size);
    ptr_copy(new_data, fmt->data, fmt->used);
    fmt->size = new_size;
    fmt->data = new_data;
    return true;
}

static void fmt_c(Fmt *fmt, u8 c) {
    if (!fmt_grow(fmt, 1)) return;
    fmt->data[fmt->used++] = c;
    if (fmt->flush_on_newline && c == '\n') {
        fmt_flush(fmt);
        fmt->opt = (Fmt_Options){};
    }
}

static void fmt_buf(Fmt *fmt, Buffer data) {
    for (size_t i = 0; i < data.size; ++i) {
        fmt_c(fmt, data.data[i]);
    }
}

static void fmt_s(Fmt *fmt, char *str) {
    if (!str) return;
    fmt_buf(fmt, str_buf(str));
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

static void fmt_u_ex(Fmt *fmt, u64 value, u32 base, u8 pad_char, u32 pad) {
    u32 digit_count = 0;
    u8 digit_list[64];

    if (fmt->opt.base) base = fmt->opt.base;

    assert(base >= 2 && base <= 16);
    if (pad > array_count(digit_list)) pad = array_count(digit_list);
    do {
        u8 digit = value % base;
        u8 chr = digit < 10 ? '0' + digit : 'a' + (digit - 10);
        digit_list[digit_count++] = chr;
        assert(digit_count <= array_count(digit_list));
        value /= base;
    } while (value > 0);

    while (digit_count < pad) {
        digit_list[digit_count++] = pad_char;
        assert(digit_count <= array_count(digit_list));
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

static void fmt_pad_line(Fmt *fmt, size_t line_len, u8 pad_char) {
    size_t line_start = fmt->used;
    size_t line_end = fmt->used;

    for (;;) {
        if (line_start == 0) break;
        if (fmt->data[line_start - 1] == '\n') {
            break;
        }
        line_start--;
    }

    for (size_t i = line_end; i < line_start + line_len; ++i) {
        fmt_c(fmt, pad_char);
    }
}

static bool chr_is_printable(u32 c) {
    return c >= 0x20 && c <= 0x7e;
}

// 2  -> 1
// 16 -> 4
static u32 u32_log2_ceil(u32 x) {
    for (u32 i = 0;; ++i) {
        x >>= 1;
        if (!x) return i;
    }
}

static void fmt_hexdump_x(Fmt *fmt, Buffer data, u32 base, u32 width) {
    // Ensure we print something when no data is passed
    size_t data_size = MAX(data.size, 1);

    // Calculate address padding
    u32 pad = 0;
    while (data_size >> pad * 4) pad += 4;
    size_t addr = 0;

    // 8 / log2(base)
    u32 pad2 = 8 / u32_log2_ceil(base);
    for (size_t addr = 0; addr < data_size; addr += width) {
        fmt_u_ex(fmt, addr, 16, ' ', pad);
        fmt_s(fmt, " | ");
        for (u32 off = 0; off < width; ++off) {
            if (addr + off >= data.size) {
                for (u32 i = 0; i < pad2 + 1; ++i) {
                    fmt_c(fmt, ' ');
                }
                continue;
            }

            fmt_u_ex(fmt, data.data[addr + off], base, '0', pad2);
            fmt_s(fmt, " ");
        }
        fmt_s(fmt, "| ");
        for (u32 off = 0; off < width; ++off) {
            if (addr + off >= data.size) {
                fmt_s(fmt, " ");
                continue;
            }
            u8 c = data.data[addr + off];
            if (!chr_is_printable(c)) c = '.';
            fmt_c(fmt, c);
        }
        fmt_s(fmt, "|\n");
    }
}

static void fmt_hexdump(Fmt *fmt, Buffer data) {
    fmt_hexdump_x(fmt, data, 16, 8);
}

// also add fmt_pad(x) -> helper for creating the opt type
// clang-format off
#define fmt1(F, x)                  \
    _Generic((x),                   \
        char *: fmt_s,              \
        size_t: fmt_u,              \
        u32: fmt_u,                 \
        i32: fmt_i,                 \
        u16: fmt_u,                 \
        i16: fmt_i,                 \
        u8: fmt_u,                  \
        i8: fmt_i,                  \
        char: fmt_c,                \
        Buffer: fmt_buf,        \
        Fmt_Options: fmt_setopt     \
    )(F, x)

#define REPEAT_1(M, A, x) M(A, x)
#define REPEAT_2(M, A, x, ...) REPEAT_1(M, A, x) __VA_OPT__(; REPEAT_1(M, A, __VA_ARGS__))
#define REPEAT_3(M, A, x, ...) REPEAT_1(M, A, x) __VA_OPT__(; REPEAT_2(M, A, __VA_ARGS__))
#define REPEAT_4(M, A, x, ...) REPEAT_1(M, A, x) __VA_OPT__(; REPEAT_3(M, A, __VA_ARGS__))
#define REPEAT_5(M, A, x, ...) REPEAT_1(M, A, x) __VA_OPT__(; REPEAT_4(M, A, __VA_ARGS__))
#define REPEAT_6(M, A, x, ...) REPEAT_1(M, A, x) __VA_OPT__(; REPEAT_5(M, A, __VA_ARGS__))
#define REPEAT_7(M, A, x, ...) REPEAT_1(M, A, x) __VA_OPT__(; REPEAT_6(M, A, __VA_ARGS__))
#define REPEAT_8(M, A, x, ...) REPEAT_1(M, A, x) __VA_OPT__(; REPEAT_7(M, A, __VA_ARGS__))
#define REPEAT_9(M, A, x, ...) REPEAT_1(M, A, x) __VA_OPT__(; REPEAT_8(M, A, __VA_ARGS__))
#define REPEAT_10(M, A, x, ...) REPEAT_1(M, A, x) __VA_OPT__(; REPEAT_9(M, A, __VA_ARGS__))
#define REPEAT(M, A, ...) REPEAT_10(M, A, __VA_ARGS__)
// clang-format on
#define fmt(F, ...) REPEAT(fmt1, F, __VA_ARGS__)

#define fstr(mem, ...) \
    ({ \
        Fmt *f = fmt_new(mem); \
        fmt(f, __VA_ARGS__); \
        fmt_end(f); \
    })

#define print(...) fmt(fout, __VA_ARGS__, "\n")
// TODO: no more 'file' in fmt, just format string
// print() will get tmp mem and free again

// Set formatting options
#define O(...) \
    (Fmt_Options) { \
        __VA_ARGS__ \
    }

#define EOL "\n"

static void fmt_setopt(Fmt *fmt, Fmt_Options opt) {
    fmt->opt = opt;
}

// Test
static void test_fmt(void) {
    Memory *mem = mem_new();
    {
        Fmt *fmt = fmt_new(mem);
        fmt_s(fmt, "Hello");
        fmt_s(fmt, " ");
        fmt_s(fmt, "World: ");
        fmt_u(fmt, 1234);
        fmt_s(fmt, "\n");
        char *output = fmt_end(fmt);
        check(str_eq(output, "Hello World: 1234\n"));
    }

    {
        Fmt *fmt = fmt_new(mem);
        fmt(fmt, "Hello", " ", "World: ", 1234, EOL);
        char *output = fmt_end(fmt);
        check(str_eq(output, "Hello World: 1234\n"));
    }

    check(str_eq(fstr(mem, "Hello", " ", "World: ", 1234, EOL), "Hello World: 1234\n"));

    // Awesome! who needs python :P
    // u32 x = 1234;
    // print("Value of X in Base10: ", x, ", Base2: ", O(.base = 2), x, "!");
    // OUTPUT: "Value of X in Base10: 1234, Base2: 10011010010!"
    mem_free(mem);
}

typedef struct {
    Memory *mem;
    u32 count;
    Buffer chunks[64];
} Fmt2;

static Fmt2 *fmt2_new(Memory *mem) {
    Fmt2 *fmt = mem_struct(mem, Fmt2);
    fmt->mem = mem;
    return fmt;
}

static Buffer fmt2_end(Fmt2 *fmt) {
    if (fmt->count == 0) return buf_null();
    if (fmt->count == 1) return fmt->chunks[0];

    // Get total size
    u32 len = 0;
    for (u32 i = 0; i < fmt->count; ++i) {
        len += fmt->chunks[i].size;
    }

    // Reserve space for output buffer
    u8 *data = mem_alloc_uninit(fmt->mem, len + 1);
    Buffer result = buf_from(data, len);

    // Fill output buffer
    u32 offset = 0;
    for (u32 i = 0; i < fmt->count; ++i) {
        ptr_copy(data + offset, fmt->chunks[i].data, fmt->chunks[i].size);
    }
    assert(offset == len);

    // Null terminate
    data[len] = 0;

    // Store result
    fmt->count = 1;
    fmt->chunks[0] = result;
    return result;
}

static void fmt2_buf(Fmt2 *fmt, Buffer buf) {
    if (fmt->count == array_count(fmt->chunks)) fmt2_end(fmt);
    fmt->chunks[fmt->count++] = buf;
}

static void fmt2_str(Fmt2 *fmt, char *str) {
    fmt2_buf(fmt, str_buf(str));
}

static char *fmt2_u64_ex(Memory *mem, bool sign, u32 base, u64 value) {
    u32 count = 0;
    char buffer[64];

    i64 value_signed = value;
    if (sign && value_signed < 0) {
        buffer[count++] = '-';
        value = -value_signed;
    }

    do {
        u32 rem = value % base;
        buffer[count++] = rem + '0';
        value /= base;
    } while (value > 0);

    char *res = mem_alloc_uninit(mem, count + 1);
    for (u32 i = 0; i < count; ++i) {
        res[i] = buffer[count - i - 1];
    }
    res[count] = 0;
    return res;
}

static char *fmt2_u32(Memory *mem, u32 value) {
    return fmt2_u64_ex(mem, 0, 10, value);
}

static char *fmt2_i32(Memory *mem, i32 value) {
    return fmt2_u64_ex(mem, 1, 10, value);
}

static char *fmt2_u64(Memory *mem, u64 value) {
    return fmt2_u64_ex(mem, 0, 10, value);
}

static char *fmt2_i64(Memory *mem, i64 value) {
    return fmt2_u64_ex(mem, 1, 10, value);
}

// ================
typedef struct {
    // Base for printing numbers
    u32 base;
    u32 zero_pad;
    u32 pad;
    bool upper;
    bool base_prefix;
} Fmt3_Options;

static void fmt3_c(Write *write, Fmt3_Options *opt, char chr) {
    write_u8(write, chr);
}

static void fmt3_s(Write *write, Fmt3_Options *opt, char *str) {
    write_buffer(write, str_buf(str));
}

static void fmt3_int(Write *write, Fmt3_Options *opt, bool is_signed, u64 value) {
    // Configuration
    u32 base = opt->base ?: 10;

    // Reversed digits
    u32 count = 0;
    char buffer[1024];
    do {
        u32 rem = value % base;
        u8 digit;
        if (rem < 10) {
            digit = rem + '0';
        } else {
            digit = rem - 10 + (opt->upper ? 'A' : 'a');
        }
        buffer[count++] = digit;
        value /= base;
    } while (value > 0);

    while (count < opt->zero_pad) {
        buffer[count++] = '0';
    }

    if (opt->base_prefix) {
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
    while (count < opt->pad) {
        buffer[count++] = ' ';
    }

    ptr_reverse(buffer, count);
    write_buffer(write, buf_from(buffer, count));
}

static void fmt3_u64(Write *write, Fmt3_Options *opt, u64 value) {
    fmt3_int(write, opt, 0, value);
}

static void fmt3_i64(Write *write, Fmt3_Options *opt, i64 value) {
    fmt3_int(write, opt, 1, value);
}

static void fmt3_buf(Write *write, Fmt3_Options *opt, Buffer buf) {
    write_buffer(write, buf);
}

static void fmt3_cfg(Write *write, Fmt3_Options *opt, Fmt3_Options set) {
    *opt = set;
}

// clang-format off
#define fmt3_1(W, x)         \
    _Generic((x),               \
        char *:       fmt3_s,   \
        i64:          fmt3_i64, \
        u64:          fmt3_u64, \
        i32:          fmt3_i64, \
        u32:          fmt3_u64, \
        i16:          fmt3_i64, \
        u16:          fmt3_u64, \
         i8:          fmt3_i64, \
         u8:          fmt3_u64, \
        char:         fmt3_c,   \
        Buffer:       fmt3_buf, \
        Fmt3_Options: fmt3_cfg  \
    )(W, &_opt, x)
// clang-format on

#define fmt3_N(F, ...) REPEAT(fmt3_1, F, __VA_ARGS__)

#define fmt3_2(F, x, ...) fmt3_1(F, x) __VA_OPT__(, fmt3_1(F, __VA_ARGS__))
#define fmt3_3(F, x, ...) fmt3_1(F, x) __VA_OPT__(, fmt3_2(F, __VA_ARGS__))
#define fmt3_4(F, x, ...) fmt3_1(F, x) __VA_OPT__(, fmt3_3(F, __VA_ARGS__))
#define fmt3_5(F, x, ...) fmt3_1(F, x) __VA_OPT__(, fmt3_4(F, __VA_ARGS__))
#define fmt3_6(F, x, ...) fmt3_1(F, x) __VA_OPT__(, fmt3_5(F, __VA_ARGS__))
#define fmt3_7(F, x, ...) fmt3_1(F, x) __VA_OPT__(, fmt3_6(F, __VA_ARGS__))
#define fmt3_8(F, x, ...) fmt3_1(F, x) __VA_OPT__(, fmt3_7(F, __VA_ARGS__))
#define fmt3_9(F, x, ...) fmt3_1(F, x) __VA_OPT__(, fmt3_8(F, __VA_ARGS__))
#define fmt3_10(F, x, ...) fmt3_1(F, x) __VA_OPT__(, fmt3_9(F, __VA_ARGS__))
#define fmt3_11(F, x, ...) fmt3_1(F, x) __VA_OPT__(, fmt3_10(F, __VA_ARGS__))
#define fmt3_12(F, x, ...) fmt3_1(F, x) __VA_OPT__(, fmt3_11(F, __VA_ARGS__))

#define write_fmt(F, ...) \
    ({ \
        Fmt3_Options _opt = {}; \
        fmt3_N(F, __VA_ARGS__); \
    })

#define fstr3(mem, ...) \
    ({ \
        Write *_write = write_new(mem); \
        Fmt3_Options _opt = {}; \
        fmt3_N(_write, __VA_ARGS__); \
        write_get_written(_write); \
    })

// TODO: no more 'file' in fmt, just format string

static void test_fmt3(void) {
    Memory *mem = mem_new();
    Buffer str = fstr3(mem, "Hello: ", 1234, 1, 2, 3, 4, 5);
    mem_free(mem);
}
