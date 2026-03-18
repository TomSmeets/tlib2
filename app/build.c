// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// build.c - Makefile but in C
#include "build.h"
#include "snake_build.h"
#include "arg.h"
#include "base64.h"
#include "command.h"
#include "fmt.h"


static void build_tl(Arg *arg) {
    os_system("mkdir -p out/tl");
    build_compile(Platform_Linux, Mode_Debug, "src/tl.c", "out/tl/tl");
}

static void build_test(Arg *arg) {
    bool gdb = arg_match(arg, "gdb", "Start with gdb");
    bool build = arg_match(arg, "build", "Build only");
    arg_help_opt(arg);

    build_compile(Platform_Linux, Mode_Debug, "src/test.c", "out/test");
    if (build) os_exit(0);

    if (gdb) {
        os_exit(os_system("DEBUGINFOD_URLS= gdb -q -ex 'b os_main' -ex 'run' -ex 'tui en' out/test"));
    } else {
        os_exit(os_system("out/test"));
    }
}
static void build_fuzz(Arg *arg) {
    os_exit(os_system("clang -Isrc -g -O2 -fsanitize=fuzzer,address src/fuzz.c -o out/fuzz && out/fuzz"));
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
        os_exit(os_system("clang-format -i --verbose */*.h */*.c"));
        return;
    }

    if (arg_match(&arg, "test", "Run Automated Tests")) {
        build_test(&arg);
        os_exit(0);
        return;
    }

    if (arg_match(&arg, "fuzz", "Run Fuzzy tests")) {
        build_fuzz(&arg);
        os_exit(0);
        return;
    }

    if (arg_match(&arg, "snake", "Build Snake")) {
        snake_build(&arg);
        os_exit(0);
        return;
    }

    if (arg_match(&arg, "tl", "Build tl cli tool")) {
        build_tl(&arg);
        os_exit(0);
        return;
    }

    if (arg_match(&arg, "lsp", "Generate compile_commands.json for autocompletion")) {
        generate_lsp(&arg);
        os_exit(0);
        return;
    }

    arg_help(&arg);
    os_exit(1);
}
