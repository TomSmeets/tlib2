// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#include "buf.h"
#include "cli.h"
#include "hot.h"
#include "os.h"
#include "parse.h"

static Memory *mem;
static Hot *hot;

// Read entire file into memory
static Buffer os_read_file(Memory *mem, char *path) {
    File *fd = os_open(path, Open_Read);
    u64 file_size = os_file_size(fd);
    u8 *file_data = mem_array(mem, u8, file_size);
    assert(os_read(fd, file_data, file_size) == file_size);
    os_close(fd);
    return (Buffer){file_data, file_size};
}

static char *fs_base(char *path) {
    u32 i = str_len(path) - 1;
    for (;;) {
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

    for (;;) {
        char *path = cli_value(cli, "<FILE>", "Input File");
        if (!path) break;

        Buffer name = str_buf(fs_base(path));
        name.size -= 2;

        Buffer buf = os_read_file(mem, path);
        Parse *parse = parse_new(mem, buf.data, buf.size);

        bool had_info = false;

        Buffer info = {};

        u32 dep_count = 0;
        Buffer deps[64];
        for (u32 i = 0;; i++) {
            if (parse_eof(parse)) break;

            if (parse_symbol(parse, "// Copyright")) {
                parse_line(parse);
                continue;
            }

            if (parse_symbol(parse, "//")) {
                Buffer line = parse_line(parse);
                if (info.data) continue;
                info = line;
                // Split on first '-' char
                for (u32 i = 0; i < info.size; ++i) {
                    if (((char *)info.data)[i] != '-') continue;
                    info.data += i + 1;
                    info.size -= i + 1;
                    break;
                }

                // Remove whitespace
                while (info.size > 0 && ((char *)info.data)[0] == ' ') {
                    info.data++;
                    info.size--;
                }
                continue;
            }

            if (parse_symbol(parse, "#pragma")) {
                parse_line(parse);
                continue;
            }

            if (parse_symbol(parse, "#include")) {
                Buffer line = parse_line(parse);
                line.data += 2;
                line.size -= 2;

                // Drop '"'
                line.size -= 1;

                // Drop '.h'
                line.size -= 2;

                deps[dep_count++] = line;
                continue;
            }
            break;
        }

        fmt_buf(fout, name.data, name.size);
        fmt_pad_line(fout, 10, ' ');
        fmt_s(fout, " | ");
        fmt_buf(fout, info.data, info.size);
        fmt_pad_line(fout, 50, ' ');
        fmt_s(fout, " | ");
        for (u32 i = 0; i < dep_count; ++i) {
            fmt_buf(fout, deps[i].data, deps[i].size);
            if (i != dep_count - 1) fmt_s(fout, ", ");
        }
        fmt_pad_line(fout, 80, ' ');
        fmt_s(fout, " |\n");
    }
    // fmt_buf(fout, buf.data, buf.size);
    os_exit(0);
}

static void cmd_run(Cli *cli) {
    if (!cli_command(cli, "run", "Run with hot reloading")) return;
    if (!hot) hot = hot_new(mem);
    hot_load(hot, "./out/main.so");

    char *argv[64] = {};
    u32 argc = cli_get_remaining(cli, array_count(argv), (char **)argv);
    hot_call(hot, argc, argv);
    hot_call(hot, argc, argv);
    hot_load(hot, "./out/main2.so");
    hot_call(hot, argc, argv);
    hot_call(hot, argc, argv);
    os_exit(0);
}

void os_main(u32 argc, char **argv) {
    if (!mem) mem = mem_new();

    Cli *cli = cli_new(mem, argc, argv);
    // cmd_run(cli);
    cmd_info(cli);
    cli_help(cli);
}
