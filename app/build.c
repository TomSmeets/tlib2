// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// build.c - Makefile but in C
#include "build.h"
#include "cli.h"
#include "command.h"
#include "fmt.h"
#include "fs.h"
#include "os_main.h"
#include "proc.h"

static void build_cmd_tl(Cli *cli) {
    cli_command(cli, "tl", "Build tl cli tool");
    bool run = cli_flag(cli, "-r", "--run", "Run directly");
    char **rest = cli_remaining(cli, "out/tl/tl");
    if (!cli_check(cli)) return;
    build_compile(Platform_Linux, Mode_Debug, "app/tl.c", "out/tl/tl");
    if (run) proc_wait(proc_exec(rest));
}

static void build_cmd_snake(Cli *cli, Memory *mem) {
    cli_command(cli, "snake", "Build Snake");
    bool quick = cli_flag(cli, "-q", "--quick", "Skip other platforms");
    bool release = cli_flag(cli, "-O", "--release", "Build in release mode");
    bool run = cli_flag(cli, "-r", "--run", "Run directly with hot reload");
    if (!cli_check(cli)) return;

    Build *build = build_new(mem, "app/snake/snake.c", "snake");
    build->release = release;
    build->linux = 1;
    if (!quick && !run) {
        build->windows = 1;
        build->wasm = !release;
        build->html = 1;
    }
    build_js(build, "lib/core/os_wasm.js");
    build_build(build);
    if (run && !error) proc_shell("out/snake/snake.elf");
}

static char *str_from_buf(Buffer buf, Memory *mem) {
    char *ret = mem_alloc_uninit(mem, buf.size + 1);
    ptr_copy(ret, buf.data, buf.size);
    ret[buf.size] = 0;
    return ret;
}

static void build_cmd_build(Cli *cli, Memory *mem) {
    cli_command(cli, "build", "Build Something");
    bool quick = cli_flag(cli, "-q", "--quick", "Skip other platforms");
    bool release = cli_flag(cli, "-O", "--release", "Build in release mode");
    bool run = cli_flag(cli, "-r", "--run", "Run directly with hot reload");
    char *path = cli_value(cli, "<SOURCE>", "main.c file");
    if (!cli_check(cli)) return;

    Path_Components split = path_split(str_buf(path));

    char *name = str_from_buf(split.base, mem);
    print("Building: ", name, " from ", path);

    Build *build = build_new(mem, path, name);
    build->release = release;
    build->linux = 1;
    if (!quick && !run) {
        build->windows = 1;
        build->wasm = !release;
        build->html = 1;
    }
    build_js(build, "lib/core/os_wasm.js");
    build_build(build);
    if (run && !error) proc_shell("out/snake/snake.elf");
}

static void build_cmd_tetris(Cli *cli, Memory *mem) {
    cli_command(cli, "tetris", "Build Tetris");
    bool quick = cli_flag(cli, "-q", "--quick", "Skip other platforms");
    bool release = cli_flag(cli, "-O", "--release", "Build in release mode");
    bool run = cli_flag(cli, "-r", "--run", "Run directly with hot reload");
    if (!cli_check(cli)) return;

    Build *build = build_new(mem, "app/tetris/tetris.c", "tetris");
    build->release = release;
    build->linux = 1;
    build_build(build);
    if (run && !error) proc_shell("out/tetris/tetris.elf");
}

static void build_cmd_tlang(Cli *cli, Memory *mem) {
    cli_command(cli, "tlang", "Build Tom's language");
    bool quick = cli_flag(cli, "-q", "--quick", "Skip other platforms");
    bool release = cli_flag(cli, "-O", "--release", "Build in release mode");
    if (!cli_check(cli)) return;

    Build *build = build_new(mem, "app/tlang/tlang.c", "tlang");
    build->release = release;
    build->linux = 1;
    build->windows = 1;
    build->wasm = 1;
    build_build(build);
}

static void build_cmd_test(Cli *cli) {
    cli_command(cli, "test", "Run Automated Tests");
    bool gdb = cli_flag(cli, "-g", "--gdb", "Start with gdb");
    bool build_only = cli_flag(cli, "-b", "--build", "Build only");
    if (!cli_check(cli)) return;

    build_compile(Platform_Linux, Mode_Debug, "app/test.c", "out/test");
    if (error) return;
    if (build_only) return;
    if (gdb) {
        proc_shell("DEBUGINFOD_URLS= gdb -q -ex 'b os_main' -ex 'run' -ex 'tui en' out/test");
    } else {
        proc_shell("out/test");
    }
}
static void build_cmd_fuzz(Cli *cli) {
    cli_command(cli, "fuzz", "Run Fuzzy tests");
    bool build_only = cli_flag(cli, "-b", "--build", "Build only");
    if (!cli_check(cli)) return;

    proc_shell("clang -std=c23 -Ilib/core -Ilib/deflate -g -O2 -fsanitize=fuzzer,address app/fuzz.c -o out/fuzz");
    if (error) return;
    if (build_only) return;
    proc_shell("out/fuzz");
}

static void build_cmd_lsp(Cli *cli) {
    cli_command(cli, "lsp", "Generate compile_commands.json for autocompletion");
    bool windows = cli_flag(cli, "-w", "--windows", "Generate for cross compiling to Windows");
    bool wasm = cli_flag(cli, "-j", "--wasm", "Generate for cross compiling to WASM");
    if (!cli_check(cli)) return;

    Build_Platform platform = Platform_Linux;
    if (windows) platform = Platform_Windows;
    if (wasm) platform = Platform_WASM;
    Command cc_cmd = build_compile_command(platform, Mode_Debug, "main.c", "out/main.elf");

    char cwd[1024];
    assert(linux_getcwd(cwd, sizeof(cwd)) > 0);

    File *fd = fs_open("compile_commands.json", FileMode_Create);
    u8 buffer[1024];

    Fmt *fmt = fmt_alloc();
    fmt_g(fmt, "[{");
    fmt_g(fmt, "\"directory\":\"", cwd, "\",");
    fmt_g(fmt, "\"command\":\"");
    for (u32 i = 0; i < cc_cmd.argc; ++i) {
        fmt_g(fmt, cc_cmd.argv[i], " ");
    }
    fmt_g(fmt, "\",");
    fmt_g(fmt, "\"file\":\"main.c\"");
    fmt_g(fmt, "}]");
    io_write(fd, fmt_end(fmt));
    fmt_free(fmt);
    io_close(fd);
}

static void build_cmd_format(Cli *cli) {
    cli_command(cli, "format", "Format all code");
    if (!cli_check(cli)) return;
    proc_shell("find . -name '*.c' -o -name '*.h' | clang-format -i --verbose --files=/dev/stdin");
}

static void os_main(void) {
    Memory *mem = mem_new();
    Cli *cli = cli_new(mem, os_argv);
    build_cmd_test(cli);
    build_cmd_fuzz(cli);
    build_cmd_snake(cli, mem);
    build_cmd_build(cli, mem);
    build_cmd_tetris(cli, mem);
    build_cmd_tl(cli);
    build_cmd_tlang(cli, mem);
    build_cmd_lsp(cli);
    build_cmd_format(cli);
    cli_help(cli);
    os_exit();
}
