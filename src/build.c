// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// build.c - Makefile but in C
#include "build.h"
#include "arg.h"
#include "base64.h"
#include "command.h"
#include "fmt.h"

static void fmt_file_contents(Fmt *fmt, char *input_path) {
    Memory *mem = mem_new();
    Buffer data = {};
    os_read_file(mem, input_path, &data);
    fmt_buf(fmt, data);
    mem_free(mem);
}

// Generate self contained html page continaing wasm module
static bool generate_html(char *output_path, char *css_path, char **js_path_list, char *wasm_path, char *html_path) {
    u8 buffer[1024 * 4];
    Fmt f = fmt_from(buffer, sizeof(buffer));
    f.file = os_open(output_path, FileMode_Create);

    fmt_s(&f, "<!DOCTYPE html>\n");
    fmt_s(&f, "<head>\n");
    if (css_path) {
        fmt_s(&f, "<style>\n");
        fmt_file_contents(&f, css_path);
        fmt_s(&f, "</style>\n");
    }

    fmt_s(&f, "<script>\n");
    for (u32 i = 0; js_path_list[i]; ++i) {
        fmt_file_contents(&f, js_path_list[i]);
    }
    fmt_s(&f, "tlib.main(Uint8Array.fromBase64(\"");
    Memory *mem = mem_new();
    Buffer buf = {};
    try(os_read_file(mem, wasm_path, &buf));
    fmt_buf(&f, base64_encode(mem, buf));
    mem_free(mem);
    fmt_s(&f, "\"));\n");
    fmt_s(&f, "</script>\n");
    fmt_s(&f, "</head>\n");
    fmt_s(&f, "<body>\n");
    fmt_file_contents(&f, html_path);
    fmt_s(&f, "</body>\n");
    fmt_end(&f);
    os_close(f.file);
    return ok();
}

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
    char *js_path[] = {"src/os_wasm.js", "src/pix_wasm.js", 0};
    char *css_path = "snake/snake.css";
    char *wasm_path = "out/snake/snake.wasm";
    char *html_path = "snake/snake.html";
    generate_html("out/snake/snake.html", css_path, js_path, wasm_path, html_path);

    // Cleanup
    if (release) os_remove("out/snake/snake.wasm");
}

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
        build_snake(&arg);
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
