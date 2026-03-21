// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// build.h - Helpers for compiling and packaging tlib applications
#pragma once
#include "base64.h"
#include "command.h"
#include "os2.h"

typedef enum {
    Platform_Windows,
    Platform_Linux,
    Platform_WASM,
} Build_Platform;

typedef enum {
    Mode_Debug,
    Mode_Release,
} Build_Mode;

// Compile using clang
static Command build_compile_command(Build_Platform platform, Build_Mode mode, char *input, char *output) {
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
    cmd_arg2(&cmd, "-I", "lib/core");
    cmd_arg2(&cmd, "-I", "lib/deflate");
    cmd_arg2(&cmd, "-I", "lib/pix");
    cmd_arg(&cmd, input);
    return cmd;
}

static bool build_compile(Build_Platform platform, Build_Mode mode, char *input, char *output) {
    Command cmd = build_compile_command(platform, mode, input, output);

    // Verbose logging
    fmt_cmd(fout, &cmd);
    fmt_s(fout, "\n");

    // Run command
    i32 ret = os_wait(os_exec(cmd.argv));
    try(ret == 0);
    return ok();
}

static bool build_lsp(Build_Platform platform, char *output) {
    Memory *mem = mem_new();
    Command cmd = build_compile_command(platform, Mode_Debug, "main.c", "out/main.elf");

    char *cwd = os_cwd(mem);

    Fmt *fmt = fmt_new(mem);
    fmt(fmt, "[{");
    fmt(fmt, "\"directory\":\"", cwd, "\",");
    fmt(fmt, "\"command\":\"");
    fmt_cmd(fmt, &cmd);
    fmt(fmt, "\",");
    fmt(fmt, "\"file\":\"main.c\"");
    fmt(fmt, "}]");
    char *out = fmt_end(fmt);
    os_write_file("compile_commands.json", str_buf(out));
    return ok();
}

// Generate self contained html page containing wasm module
static bool generate_html(char *output_path, char *css_path, char **js_path_list, char *wasm_path, char *html_path) {
    Memory *mem = mem_new();

    Fmt *f = fmt_new(mem);
    fmt(f, "<!DOCTYPE html>\n");
    fmt(f, "<head>\n");
    if (css_path) {
        fmt(f, "<style>\n");
        fmt(f, os_read_file_string(mem, css_path));
        fmt(f, "</style>\n");
    }

    fmt(f, "<script>\n");
    for (u32 i = 0; js_path_list[i]; ++i) {
        fmt(f, os_read_file_string(mem, js_path_list[i]));
    }
    fmt(f, "tlib.main(Uint8Array.fromBase64(\"");
    Buffer buf = {};
    try(os_read_file(mem, wasm_path, &buf));
    fmt_buf(f, base64_encode(mem, buf));
    mem_free(mem);
    fmt(f, "\"));\n");
    fmt(f, "</script>\n");

    fmt(f, "</head>\n");
    fmt(f, "<body>\n");
    fmt(f, os_read_file_string(mem, html_path));
    fmt(f, "</body>\n");
    os_write_file(output_path, str_buf(fmt_end(f)));
    return ok();
}

typedef struct {
    Memory *mem;

    // platforms
    bool linux;
    bool windows;
    bool wasm;
    bool html;
    bool release;

    char *output_name;
    char *source_file;

    u32 include_count;
    char *include_list[64];

    u32 js_count;
    char *js_files[64];

    u32 css_count;
    char *css_files[64];

    u32 html_count;
    char *html_files[64];
} Build;

static Build *build_new(Memory *mem, char *source_file, char *output_name) {
    Build *build = mem_struct(mem, Build);
    build->mem = mem;
    build->source_file = source_file;
    build->output_name = output_name;
    build->linux = 1;
    return build;
}

static void build_include(Build *build, char *path) {
    build->include_list[build->include_count++] = path;
}

static void build_js(Build *build, char *path) {
    build->js_files[build->js_count++] = path;
}

static void build_css(Build *build, char *path) {
    build->css_files[build->css_count++] = path;
}

static void build_html(Build *build, char *path) {
    build->html_files[build->html_count++] = path;
}

static bool build_build(Build *build) {
    Build_Mode mode = build->release ? Mode_Release : Mode_Debug;
    Memory *mem = build->mem;

    char *out_path = fstr(mem, "out/", build->output_name);
    char *out_exe = fstr(mem, out_path, "/", build->output_name, ".exe");
    char *out_elf = fstr(mem, out_path, "/", build->output_name, ".elf");
    char *out_wasm = fstr(mem, out_path, "/", build->output_name, ".wasm");
    char *out_html = fstr(mem, out_path, "/", build->output_name, ".html");

    os_system(fstr(mem, "mkdir -p ", out_path));
    if (error) return 0;

    if (build->windows) build_compile(Platform_Windows, mode, build->source_file, out_exe);
    if (build->linux) build_compile(Platform_Linux, mode, build->source_file, out_elf);
    if (build->wasm || build->html) build_compile(Platform_WASM, mode, build->source_file, out_wasm);

    if (build->html) {
        Fmt *f = fmt_new(mem);
        fmt(f, "<!DOCTYPE html>\n");
        fmt(f, "<head>\n");
        for (u32 i = 0; i < build->css_count; ++i) {
            fmt(f, "<style>\n");
            fmt(f, os_read_file_string(mem, build->css_files[i]));
            fmt(f, "</style>\n");
        }

        fmt(f, "<script>\n");

        // Embed js files
        for (u32 i = 0; i < build->js_count; ++i) {
            fmt(f, "// ", build->js_files[i], "\n");
            fmt(f, os_read_file_string(mem, build->js_files[i]));
        }

        // Load embedded wasm file
        fmt(f, "tlib.main(Uint8Array.fromBase64(\"");
        Buffer buf = {};
        try(os_read_file(mem, out_wasm, &buf));
        fmt_buf(f, base64_encode(mem, buf));
        mem_free(mem);
        fmt(f, "\"));\n");
        fmt(f, "</script>\n");

        fmt(f, "</head>\n");
        fmt(f, "<body>\n");
        for (u32 i = 0; i < build->html_count; ++i) {
            fmt(f, os_read_file_string(mem, build->html_files[i]));
        }
        fmt(f, "</body>\n");
        os_write_file(out_html, str_buf(fmt_end(f)));
        if (!build->wasm) os_remove(out_wasm);
    }
    return ok();
}
