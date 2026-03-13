// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// build.h - Helpers for compiling and packaging tlib applications
#pragma once
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
    cmd_arg2(&cmd, "-I", "src");
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
