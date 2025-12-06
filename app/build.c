// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#include "buf.h"
#include "cli.h"
#include "hot.h"
#include "os.h"
#include "parse.h"

static Memory *mem;
static Hot *hot;

// Read entire file into memory
static Buffer os_read_file(Memory *mem, const char *path) {
    File *fd = os_open(path, Open_Read);
    u64 file_size = os_file_size(fd);
    u8 *file_data = mem_array(mem, u8, file_size);
    assert(os_read(fd, file_data, file_size) == file_size);
    os_close(fd);
    return (Buffer){file_data, file_size};
}

static const char *fs_base(const char *path) {
    u32 i = str_len(path) - 1;
    for(;;) {
        if (path[i] == '/') return path + i + 1;
        if (i == 0) break;
        i--;
    }
    return path;
}

// Generate a table
// file | info | deps
static void cmd_info(Cli *cli) {
    if (!cli_command(cli, "info", "Show information about a c file")) return;
    const char *path = cli_value(cli, "<FILE>", "Input File");
    if (!path) cli_cmdhelp(cli);

    const char *name = fs_base(path);
    fmt_ss(fmtout, "FILE: ", path, "\n");

    fmt_s(fout, name);
    fmt_s(fout, " | ");

    Buffer buf = os_read_file(mem, path);
    Parse *parse = parse_new(mem, buf.data, buf.size);
    for (u32 i = 0;; i++) {
        if (parse_eof(parse)) break;

        if(parse_symbol(parse, "// Copyright")) {
            parse_line(parse);
            continue;
        }

        if(parse_symbol(parse, "//")) {
            Buffer line = parse_line(parse);
            fmt_buf(fout, line.data, line.size);
            fmt_s(fout, " | ");
            continue;
        }

        if(parse_symbol(parse, "#pragma")) {
            parse_line(parse);
            continue;
        }

        if(parse_symbol(parse, "#include")) {
            Buffer line = parse_line(parse);
            line.data += 2;
            line.size -= 3;
            fmt_buf(fout, line.data, line.size);
            fmt_s(fout, " ");
            continue;
        }
        break;
    }
    fmt_s(fout, "\n");
    // fmt_buf(fmtout, buf.data, buf.size);
    os_exit(0);
}

static void cmd_run(Cli *cli) {
    if (!cli_command(cli, "run", "Run with hot reloading")) return;
    if (!hot) hot = hot_new(mem);
    hot_load(hot, "./out/main.so");
    hot_call(hot, cli->argc, cli->argv);
    hot_call(hot, cli->argc, cli->argv);
    hot_load(hot, "./out/main2.so");
    hot_call(hot, cli->argc, cli->argv);
    hot_call(hot, cli->argc, cli->argv);
    os_exit(0);
}

void os_main(u32 argc, const char **argv) {
    if (!mem) mem = mem_new();

    Cli *cli = cli_new(mem, argc, argv);
    cmd_run(cli);
    cmd_info(cli);
    cli_help(cli);
}
