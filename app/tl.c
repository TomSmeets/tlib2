// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// tl.c - Cli utils using tlib
#include "arg.h"
#include "base64.h"
#include "dwarf.h"
#include "elf.h"
#include "gzip.h"
#include "os.h"
#include "stream.h"

static Buffer os_read_full(Memory *mem, File *file) {
    Stream *input = stream_new(mem);
    stream_write_from_file(input, file);
    return stream_as_buffer(input);
}

static void os_write_full(File *file, Buffer data) {
    while (data.size) {
        size_t written = os_write(file, data);
        data = buf_drop(data, written);
        if (error) return;
    }
}

void os_main(u32 argc, char **argv) {
    Memory *mem = mem_new();
    Arg arg = arg_new(argc, argv);

    if (arg_match(&arg, "hello", "Print Hello World")) {
        print("Hello World!");
        os_exit();
    }

    if (arg_match(&arg, "base64", "Encode / Decode base64")) {
        bool encode = arg_match(&arg, "encode", "Encode Base64 data");
        bool decode = arg_match(&arg, "decode", "Decode Base64 data");
        if (!encode) decode = 1;
        arg_help_opt(&arg);

        Buffer input = os_read_full(mem, os_stdin());
        if (error) os_exit();

        Buffer output = {};
        if (encode) output = base64_encode(mem, input);
        if (decode) output = base64_decode(mem, input);
        check(output.data);
        if (error) os_exit();

        os_write_full(os_stdout(), output);
        os_exit();
    }

    if (arg_match(&arg, "gzip", "Read / Write Gzip")) {
        bool compress = arg_match(&arg, "compress", "Encode Base64 data");
        bool decompress = arg_match(&arg, "decompress", "Decode Base64 data");
        if (!compress) decompress = 1;
        arg_help_opt(&arg);

        Buffer input = os_read_full(mem, os_stdin());
        check(input.size);
        if (error) os_exit();

        Buffer output = {};
        if (compress) output = gzip_write(mem, input);
        if (decompress) output = gzip_read(mem, input);
        check(output.size);
        if (error) os_exit();
        os_write_full(os_stdout(), output);
        os_exit();
    }

    if (arg_match(&arg, "dump", "Hexdump")) {
        bool bin = arg_match(&arg, "bin", "Base 2");
        bool hex = arg_match(&arg, "hex", "Base 16");
        bool wide = arg_match(&arg, "wide", "Wide");
        bool compact = arg_match(&arg, "compact", "Less wide");
        if (!bin && !hex) hex = 1;
        if (!wide && !compact) wide = hex;
        arg_help_opt(&arg);

        Buffer input = os_read_full(mem, os_stdin());
        u32 base = hex ? 16 : 2;
        u32 width = wide ? 16 : 4;
        fmt_hexdump_x(fout, input, base, width);
        os_exit();
    }

    if (arg_match(&arg, "elf", "ELF")) {
        char *path = arg_next(&arg);
        File *file = os_open(path, FileMode_Read);
        Elf *elf = elf_load(mem, file);
        if (error) os_exit();
        print("entry: 0x", O(.base = 16), elf->entry);
        for (u32 i = 0; i < elf->section_count; ++i) {
            print(i, " ", elf->sections[i].size, " ", elf->sections[i].name);
        }

        dwarf_load(mem, elf);

        os_exit();
    }

    arg_help(&arg);
    os_fail("");
}
