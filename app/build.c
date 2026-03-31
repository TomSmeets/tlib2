// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// build.c - Makefile but in C
#include "build.h"
#include "arg.h"
#include "command.h"
#include "fmt.h"
#include "cli.h"
#include "snake/snake_build.h"

static void  build_cmd_tl(Cli *cli) {
    cli_command(cli, "tl", "Build tl cli tool");
    bool run = cli_flag(cli, "-r", "--run", "Run directly");
    char **rest = cli_remaining(cli, "out/tl/tl");
    if(!cli_check(cli)) return;
    build_compile(Platform_Linux, Mode_Debug, "app/tl.c", "out/tl/tl");
    if(run) os_wait(os_exec(rest));
}

static void build_cmd_snake(Cli *cli, Memory *mem) {
    cli_command(cli, "snake", "Build Snake");
    bool quick = cli_flag(cli, "-q", "--quick", "Skip other platforms");
    bool release = cli_flag(cli, "-O", "--release", "Build in release mode");
    bool run = cli_flag(cli, "-r", "--run", "Run snake directly with hot reload");
    if(!cli_check(cli)) return;

    Build *build = build_new(mem, "app/snake/snake.c", "snake");
    build->release = release;
    build->linux = 1;
    if (!quick && !run) {
        build->windows = 1;
        build->wasm = !release;
        build->html = 1;
    }
    build_js(build, "lib/core/os_wasm.js");
    build_js(build, "lib/pix/pix_wasm.js");
    build_js(build, "app/snake/snake.js");
    build_css(build, "app/snake/snake.css");
    build_html(build, "app/snake/snake.html");
    build_build(build);
    if (run && !error) os_system("out/snake/snake.elf");
}

static void build_cmd_tlang(Cli *cli, Memory *mem) {
    cli_command(cli, "tlang", "Build Tom's language");
    bool quick = cli_flag(cli, "-q", "--quick", "Skip other platforms");
    bool release = cli_flag(cli, "-O", "--release", "Build in release mode");
    if(!cli_check(cli)) return;

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
    if(!cli_check(cli)) return;

    build_compile(Platform_Linux, Mode_Debug, "app/test.c", "out/test");
    if (error) return;
    if(build_only) return;
    if (gdb) {
        os_system("DEBUGINFOD_URLS= gdb -q -ex 'b os_main' -ex 'run' -ex 'tui en' out/test");
    } else {
        os_system("out/test");
    }
}
static void build_cmd_fuzz(Cli *cli) {
    cli_command(cli, "fuzz", "Run Fuzzy tests");
    bool build_only = cli_flag(cli, "-b", "--build", "Build only");
    if(!cli_check(cli)) return;

    os_system("clang -std=c23 -Ilib/core -Ilib/deflate -g -O2 -fsanitize=fuzzer,address app/fuzz.c -o out/fuzz");
    if (error) return;
    if (build_only) return;
    os_system("out/fuzz");
}

static void build_cmd_lsp(Cli *cli) {
    cli_command(cli, "lsp", "Generate compile_commands.json for autocompletion");
    bool windows = cli_flag(cli, "-w", "--windows", "Generate for cross compiling to Windows");
    bool wasm = cli_flag(cli, "-j", "--wasm", "Generate for cross compiling to WASM");
    if(!cli_check(cli)) return;

    Build_Platform platform = Platform_Linux;
    if (windows) platform = Platform_Windows;
    if (wasm) platform = Platform_WASM;
    Command cc_cmd = build_compile_command(platform, Mode_Debug, "main.c", "out/main.elf");

    char cwd[1024];
    assert(linux_getcwd(cwd, sizeof(cwd)) > 0);

    File *fd = os_open("compile_commands.json", FileMode_Create);
    u8 buffer[1024];
    Fmt f = {.file = fd, .data = buffer, .size = sizeof(buffer)};
    fmt(&f, "[");
    fmt(&f, "{");
    fmt(&f, "\"directory\":\"", cwd, "\",");
    fmt(&f, "\"command\":\"");
    for (u32 i = 0; i < cc_cmd.argc; ++i) {
        fmt(&f, cc_cmd.argv[i]);
        fmt(&f, " ");
    }
    fmt(&f, "\",");
    fmt(&f, "\"file\":\"main.c\"");
    fmt(&f, "}");
    fmt(&f, "]");
    fmt_end(&f);
    os_close(fd);
}

static void build_cmd_format(Cli *cli) {
    cli_command(cli, "format", "Format all code");
    if (!cli_check(cli)) return;
    os_system("find . -name '*.c' -o -name '*.h' | clang-format -i --verbose --files=/dev/stdin");
}

void os_main(u32 argc, char **argv) {
    Memory *mem = mem_new();
    Cli *cli = cli_new(mem, argv);

    build_cmd_test(cli);
    build_cmd_fuzz(cli);
    build_cmd_snake(cli, mem);
    build_cmd_tl(cli);
    build_cmd_tlang(cli, mem);
    build_cmd_lsp(cli);
    cli_help(cli);
    os_exit();
}
