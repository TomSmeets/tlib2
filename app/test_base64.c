#include "fmt.h"
#include "os.h"
#include "type.h"

const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Convert a byte to the correct base64 char
// - Considers only the bottom 6 bits
static u8 base64_from_bits(u8 c) {
    // Only consider lower 6  bits
    c &= 63;

    if (c < 26) return c - 0 + 'A';
    if(c < 52) return c - 26 + 'a';
    if(c < 62) return c - 52 + '0';
    if(c == 62) return '+';
    if(c == 63) return '/';

    // invalid
    return 0;
}

// Convert a base64 char to bits
// returns a 6 bit value
static u8 base64_to_bits(u8 c) {
    if(c >= 'A' && c <= 'Z') return c - 'A' + 0;
    if(c >= 'a' && c <= 'z') return c - 'a' + 26;
    if(c >= '0' && c <= '9') return c - '0' + 52;
    if(c == '+') return 62;
    if(c == '/') return 63;

    // invalid
    return 0;
}

static Buffer base64_encode(Memory *mem, Buffer input) {
    // Converts: 3 * 8 bit -> 4 * 6 bit
    //
    // Every 3 bytes are converted to 4 characters
    // Round up, the rest is padded wiht '='
    //
    //          <-----  Cunk  ------>
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
        if (j < output_count) output_data[j++] = base64_from_bits(chunk_value >> (6 * 3)); else output_data[j++] = '=';
        if (j < output_count) output_data[j++] = base64_from_bits(chunk_value >> (6 * 2)); else output_data[j++] = '=';
        if (j < output_count) output_data[j++] = base64_from_bits(chunk_value >> (6 * 1)); else output_data[j++] = '=';
        if (j < output_count) output_data[j++] = base64_from_bits(chunk_value >> (6 * 0)); else output_data[j++] = '=';
        assert(j <= buffer_size);
    }
    return (Buffer){output_data, buffer_size};
}

// Decode base64 data
// Trailing '=' are optional
// Returns zero terminated data
// NOTE: i prefer writing to a buffer first, this makes code more flexible and simpler
// NOTE: Don't write to a fmt first
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

static void base64_test_encode(Buffer input, char *expect) {
    Memory *mem = mem_new();
    char *output  = base64_encode(mem, input).data;
    assert(str_eq(output, expect));
    Buffer reverse = base64_decode(mem, str_buf(output));
    assert(buf_eq(input, reverse));
    mem_free(mem);
}

static void base64_test(void) {
    base64_test_encode(str_buf("Hello World!"), "SGVsbG8gV29ybGQh");
    base64_test_encode(str_buf(""), "");
    base64_test_encode(str_buf("a"), "YQ==");
    base64_test_encode(str_buf("aa"), "YWE=");
    base64_test_encode(str_buf("aaa"), "YWFh");
    base64_test_encode(str_buf("aaaa"), "YWFhYQ==");

    u8 test_data0[] = { 0, 0, 0, 0 };
    u8 test_data1[] = { 1, 2, 3, 4, 5, 6 };
    base64_test_encode((Buffer){test_data0, sizeof(test_data0)}, "AAAAAA==");
    base64_test_encode((Buffer){test_data1, sizeof(test_data1)}, "AQIDBAUG");
}

void os_main(u32 argc, char **argv) {
    base64_test();
    for (u32 i = 1; i < argc; ++i) {
        Memory *mem = mem_new();

        Buffer input  = str_buf(argv[i]);
        Buffer output = base64_encode(mem, input);
        fmt_ss(fout, "> ", output.data, "\n");
        mem_free(mem);
    }
    os_exit(0);
}
