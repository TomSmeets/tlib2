// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// build2.c - Makefile but in C
#include "arg.h"
#include "base64.h"
#include "command.h"
#include "fmt.h"

typedef enum {
    Platform_Windows,
    Platform_Linux,
    Platform_WASM,
} Platform;

typedef enum {
    Mode_Debug,
    Mode_Release,
} Mode;

static Command clang_compile_command(Platform platform, Mode mode, char **include, char *input, char *output) {
    Command cmd = {};
    cmd_arg(&cmd, "clang");

    // Enforce C23 standard
    cmd_arg(&cmd, "-std=c23");

    // Warn flags
    cmd_arg(&cmd, "-Wall");
    cmd_arg(&cmd, "-Werror");
    cmd_arg(&cmd, "-Wno-unused-function");
    cmd_arg(&cmd, "-Wno-unused-variable");
    cmd_arg(&cmd, "-Wno-unused-but-set-variable");

    // Mode
    if (mode == Mode_Debug) {
        cmd_arg(&cmd, "-g");
        cmd_arg(&cmd, "-O0");
    }

    if (mode == Mode_Release) {
        cmd_arg(&cmd, "-g0");
        cmd_arg(&cmd, "-O2");
        cmd_arg2(&cmd, "-Xlinker", "--strip-all");
    }

    if (platform == Platform_Windows) {
        cmd_arg2(&cmd, "-target", "x86_64-unknown-windows-gnu");
    }

    if (platform == Platform_WASM) {
        cmd_arg2(&cmd, "-target", "wasm32");
        cmd_arg(&cmd, "--no-standard-libraries");
        cmd_arg(&cmd, "-Wl,--no-entry");
        cmd_arg(&cmd, "-Wl,--export-all");
        cmd_arg(&cmd, "-fno-builtin");
        cmd_arg(&cmd, "-msimd128");
    }

    cmd_arg2(&cmd, "-o", output);
    for (u32 i = 0; include[i]; ++i) {
        cmd_arg2(&cmd, "-I", include[i]);
    }
    cmd_arg(&cmd, input);
    return cmd;
}

static void clang_compile(Platform platform, Mode mode, char **include, char *input, char *output) {
    Command cmd = clang_compile_command(platform, mode, include, input, output);
    fmt_cmd(fout, &cmd);
    fmt_s(fout, "\n");
    i32 ret = os_wait(os_exec(cmd.argv));
    if (ret != 0) os_exit(ret);
}

// Read entire file into memory
static Buffer os_read_file(Memory *mem, char *path) {
    FileInfo info = {};
    assert(os_stat(path, &info));

    File *fd = os_open(path, FileMode_Read);
    u8 *file_data = mem_array(mem, u8, info.size);
    u64 bytes_read = 0;
    assert(os_read(fd, file_data, info.size, &bytes_read));
    assert(bytes_read == info.size);
    assert(os_close(fd));
    return (Buffer){file_data, info.size};
}

static void fmt_file_contents(Fmt *fmt, char *input_path) {
    Memory *mem = mem_new();
    Buffer data = os_read_file(mem, input_path);
    fmt_buf(fmt, data);
    mem_free(mem);
}

// Generate self contained html page continaing wasm module
static void generate_html(char *output_path, char *css_path, char **js_path_list, char *wasm_path, char *html_path) {
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
    Buffer buf = os_read_file(mem, wasm_path);
    fmt_buf(&f, base64_encode(mem, os_read_file(mem, wasm_path)));
    mem_free(mem);
    fmt_s(&f, "\"));\n");
    fmt_s(&f, "</script>\n");

    fmt_s(&f, "</head>\n");
    fmt_s(&f, "<body>\n");
    fmt_file_contents(&f, html_path);
    fmt_s(&f, "</body>\n");
    fmt_end(&f);
    os_close(f.file);
}

static void build_snake(Arg *arg) {
    bool quick = arg_match(arg, "quick", "Skip other platforms");
    bool release = arg_match(arg, "release", "Build in release mode");
    bool run = arg_match(arg, "run", "Run snake directly with hot reload");
    arg_help_opt(arg);

    os_system("mkdir -p out/snake");
    Mode mode = Mode_Debug;
    if (release) mode = Mode_Release;

    char *include[] = {"core", "gfx", 0};
    clang_compile(Platform_Linux, mode, include, "snake/snake.c", "out/snake/snake.elf");

    if (run) os_exit(os_system("out/snake/snake.elf"));
    if (quick) return;
    clang_compile(Platform_Windows, mode, include, "snake/snake.c", "out/snake/snake.exe");
    clang_compile(Platform_WASM, mode, include, "snake/snake.c", "out/snake/snake.wasm");

    // Generate html page
    char *js_path[] = {"core/os_wasm.js", "gfx/pix_wasm.js", 0};
    char *css_path = "snake/snake.css";
    char *wasm_path = "out/snake/snake.wasm";
    char *html_path = "snake/snake.html";
    generate_html("out/snake/snake.html", css_path, js_path, wasm_path, html_path);

    // Cleanup
    if (release) os_remove("out/snake/snake.wasm");
}

static void build_tl(Arg *arg) {
    os_system("mkdir -p out/tl");
    char *include[] = {"core", 0};
    clang_compile(Platform_Linux, Mode_Debug, include, "app/tl.c", "out/tl/tl");
}

static void build_test(Arg *arg) {
    bool gdb = arg_match(arg, "gdb", "Start with gdb");
    bool build = arg_match(arg, "build", "Build only");
    arg_help_opt(arg);

    char *include[] = {"core", 0};
    clang_compile(Platform_Linux, Mode_Debug, include, "app/test.c", "out/test");
    if(build) os_exit(0);

    if (gdb) {
        os_exit(os_system("DEBUGINFOD_URLS= gdb -q -ex 'b os_main' -ex 'run' -ex 'tui en' out/test"));
    } else {
        os_exit(os_system("out/test"));
    }
}

static void generate_lsp(Arg *arg) {
    char *include[] = {"core", "gfx", 0};
    bool windows = arg_match(arg, "windows", "Generate for cross compiling to Windows");
    bool wasm = arg_match(arg, "wasm", "Generate for cross compiling to WASM");
    arg_help_opt(arg);

    Platform platform = Platform_Linux;
    if (windows) platform = Platform_Windows;
    if (wasm) platform = Platform_WASM;
    Command cmd = clang_compile_command(platform, Mode_Debug, include, "main.c", "out/main.elf");

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
