// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// build.c - Makefile but in C
#include "build.h"
#include "arg.h"
#include "command.h"
#include "fmt.h"
#include "snake/snake_build.h"

static void build_tl(Arg *arg) {
    if (!arg_match(arg, "tl", "Build tl cli tool")) return;
    os_system("mkdir -p out/tl");
    build_compile(Platform_Linux, Mode_Debug, "app/tl.c", "out/tl/tl");
    os_exit();
}

static void snake_build(Arg *arg) {
    if (!arg_match(arg, "snake", "Build Snake")) return;
    bool quick = arg_match(arg, "quick", "Skip other platforms");
    bool release = arg_match(arg, "release", "Build in release mode");
    bool run = arg_match(arg, "run", "Run snake directly with hot reload");

    // Show help message if unknown argument was passed
    arg_help_opt(arg);

    Memory *mem = mem_new();
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
    if (error) os_exit();
    if (run) os_system("out/snake/snake.elf");
    os_exit();
}

static void build_test(Arg *arg) {
    bool gdb = arg_match(arg, "gdb", "Start with gdb");
    bool build = arg_match(arg, "build", "Build only");
    arg_help_opt(arg);

    build_compile(Platform_Linux, Mode_Debug, "app/test.c", "out/test");
    if (error) os_exit();
    if (build) os_exit();

    if (gdb) {
        os_system("DEBUGINFOD_URLS= gdb -q -ex 'b os_main' -ex 'run' -ex 'tui en' out/test");
    } else {
        os_system("out/test");
    }
    os_exit();
}
static void build_fuzz(Arg *arg) {
    os_system("clang -Ilib/core -Ilib/deflate -g -O2 -fsanitize=fuzzer,address app/fuzz.c -o out/fuzz && out/fuzz");
    os_exit();
}

static void generate_lsp(Arg *arg) {
    bool windows = arg_match(arg, "windows", "Generate for cross compiling to Windows");
    bool wasm = arg_match(arg, "wasm", "Generate for cross compiling to WASM");
    arg_help_opt(arg);

    Build_Platform platform = Platform_Linux;
    if (windows) platform = Platform_Windows;
    if (wasm) platform = Platform_WASM;
    Command cmd = build_compile_command(platform, Mode_Debug, "main.c", "out/main.elf");

    char cwd[1024];
    assert(linux_getcwd(cwd, sizeof(cwd)) > 0);

    File *fd = os_open("compile_commands.json", FileMode_Create);
    u8 buffer[1024];
    Fmt fmt = {.file = fd, .data = buffer, .size = sizeof(buffer)};
    fmt_s(&fmt, "[");
    fmt_s(&fmt, "{");
    fmt_ss(&fmt, "\"directory\":\"", cwd, "\",");
    fmt_s(&fmt, "\"command\":\"");
    for (u32 i = 0; i < cmd.argc; ++i) {
        fmt_s(&fmt, cmd.argv[i]);
        fmt_s(&fmt, " ");
    }
    fmt_s(&fmt, "\",");
    fmt_s(&fmt, "\"file\":\"main.c\"");
    fmt_s(&fmt, "}");
    fmt_s(&fmt, "]");
    fmt_end(&fmt);
    os_close(fd);
}

void os_main(u32 argc, char **argv) {
    Arg arg = {argc, argv, 1};

    if (arg_match(&arg, "format", "Format all code")) {
        os_system("find . -name '*.c' -o -name '*.h' | clang-format -i --verbose --files=/dev/stdin");
        os_exit();
    }

    if (arg_match(&arg, "test", "Run Automated Tests")) {
        build_test(&arg);
        os_exit();
    }

    if (arg_match(&arg, "fuzz", "Run Fuzzy tests")) {
        build_fuzz(&arg);
        os_exit();
    }

    snake_build(&arg);
    build_tl(&arg);

    if (arg_match(&arg, "lsp", "Generate compile_commands.json for autocompletion")) {
        generate_lsp(&arg);
        os_exit();
    }

    arg_help(&arg);
    os_exit();
}
