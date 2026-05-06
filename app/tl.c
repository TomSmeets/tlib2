// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// tl.c - Cli utils using tlib
#include "base64.h"
#include "cli.h"
#include "dwarf.h"
#include "elf.h"
#include "gzip.h"
#include "os.h"

static void tl_cmd_hello(Cli *cli) {
    cli_command(cli, "hello", "Print Hello World");
    if (!cli_check(cli)) return;
    print("Hello World!");
}

static void tl_cmd_base64(Cli *cli, Memory *mem) {
    cli_command(cli, "base64", "Encode / Decode base64");
    bool encode = cli_flag(cli, "-e", "--encode", "Encode Base64 data");
    bool decode = cli_flag(cli, "-d", "--decode", "Decode Base64 data");
    if (!encode && !decode) encode = 1;
    if (!cli_check(cli)) return;

    Buffer input = io_read(mem, io_stdin());
    Buffer output = {};
    if (encode) output = base64_encode(mem, input);
    if (decode) output = base64_decode(mem, input);
    io_write(io_stdout(), output);
}

static void tl_cmd_gzip(Cli *cli, Memory *mem) {
    cli_command(cli, "gzip", "Read / Write Gzip files");
    bool compress = cli_flag(cli, "-c", "--compress", "Compress data to a GZip file");
    bool decompress = cli_flag(cli, "-d", "--decompress", "Decompress a GZip file");
    if (!compress && !decompress) compress = 1;
    if (!cli_check(cli)) return;

    Buffer input = io_read(mem, io_stdin());
    Buffer output = {};
    if (compress) output = gzip_write(mem, input);
    if (decompress) output = gzip_read(mem, input);
    io_write(io_stdout(), output);
}

static void tl_cmd_dump(Cli *cli, Memory *mem) {
    cli_command(cli, "dump", "Hexdump");
    bool bin = cli_flag(cli, "-b", "--bin", "Base 2");
    bool hex = cli_flag(cli, "-h", "--hex", "Base 16");
    bool wide = cli_flag(cli, "-w", "--wide", "Wide output (16 bytes per row)");
    bool compact = cli_flag(cli, "-c", "--compact", "Compact output (4 bytes per row)");
    if (!bin && !hex) hex = 1;
    if (!wide && !compact) wide = hex;
    if (!cli_check(cli)) return;

    Buffer input = io_read_all(mem, io_stdin());
    u32 base = hex ? 16 : 2;
    u32 width = wide ? 16 : 4;

    Fmt *fmt = fmt_alloc();
    fmt_base(fmt, base);
    fmt_hexdump(fmt, input, width);
    io_write(io_stdout(), fmt_end(fmt));
    fmt_free(fmt);
}

static void tl_cmd_elf(Cli *cli, Memory *mem) {
    cli_command(cli, "elf", "Elf and dwarf reader");
    char *path = cli_value(cli, "<Input>", "Input File");
    if (!cli_check(cli)) return;

    File *file = io_open(path, FileMode_Read);
    Elf *elf = elf_load(mem, file);
    if (error) return;

    print("entry: 0x", F_Base(16), elf->entry);
    for (u32 i = 0; i < elf->section_count; ++i) {
        print(i, " ", elf->sections[i].size, " ", elf->sections[i].name);
    }
    dwarf_load(mem, elf);
}

static void os_main(void) {
    Memory *mem = mem_new();
    Cli *cli = cli_new(mem, os_argv);
    tl_cmd_hello(cli);
    tl_cmd_base64(cli, mem);
    tl_cmd_gzip(cli, mem);
    tl_cmd_dump(cli, mem);
    tl_cmd_elf(cli, mem);
    cli_help(cli);
    os_exit();
}
