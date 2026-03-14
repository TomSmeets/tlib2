// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// build.c - Makefile but in C
#include "build.h"
#include "arg.h"
#include "command.h"
#include "fmt.h"

static void build_snake(Arg *arg) {
    bool quick = arg_match(arg, "quick", "Skip other platforms");
    bool release = arg_match(arg, "release", "Build in release mode");
    bool run = arg_match(arg, "run", "Run snake directly with hot reload");
    arg_help_opt(arg);

    os_system("mkdir -p out/snake");
    Build_Mode mode = Mode_Debug;
    if (release) mode = Mode_Release;

    build_compile(Platform_Linux, mode, "snake/snake.c", "out/snake/snake.elf");

    if (run) os_exit(os_system("out/snake/snake.elf"));
    if (quick) return;
    build_compile(Platform_Windows, mode, "snake/snake.c", "out/snake/snake.exe");
    build_compile(Platform_WASM, mode, "snake/snake.c", "out/snake/snake.wasm");

    // Generate html page
    char *js_path[] = {"src/os_wasm.js", "src/pix_wasm.js", "snake/snake.js", 0};
    char *css_path = "snake/snake.css";
    char *wasm_path = "out/snake/snake.wasm";
    char *html_path = "snake/snake.html";
    generate_html("out/snake/snake.html", css_path, js_path, wasm_path, html_path);

    // Cleanup
    if (release) os_remove("out/snake/snake.wasm");
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

    if (arg_match(&arg, "snake", "Build Snake")) {
        build_snake(&arg);
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
