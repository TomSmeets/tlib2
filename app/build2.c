#include "base64.h"
#include "fmt.h"

// ============== CMD =========================
typedef struct {
    u32 argc;
    char *argv[256];
} Command;

static void cmd_arg(Command *cmd, char *arg) {
    cmd->argv[cmd->argc++] = arg;
    cmd->argv[cmd->argc] = 0;
}

static void cmd_arg2(Command *cmd, char *arg1, char *arg2) {
    cmd_arg(cmd, arg1);
    cmd_arg(cmd, arg2);
}

static void fmt_cmd(Fmt *fmt, Command *cmd) {
    for (u32 i = 0; i < cmd->argc; ++i) {
        if (i > 0) fmt_s(fout, " ");
        fmt_s(fmt, cmd->argv[i]);
    }
}

// ========= Clang =================
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

typedef struct {
    u32 argc;
    char **argv;
    u32 i;

    u32 opt_count;
    char *opts[64][2];
} Arg;

static Arg arg_new(u32 argc, char **argv) {
    return (Arg){argc, argv, 1};
}

static bool arg_match(Arg *arg, char *name, char *info) {
    arg->opts[arg->opt_count][0] = name;
    arg->opts[arg->opt_count][1] = info;
    arg->opt_count++;

    if (arg->i >= arg->argc) return false;
    if (!str_eq(arg->argv[arg->i], name)) return false;
    arg->i++;
    arg->opt_count = 0;
    return true;
}

static void arg_help(Arg *arg, Fmt *fmt) {
    fmt_s(fmt, "Usage: ");
    for (u32 i = 0; i < arg->i; ++i) {
        fmt_s(fmt, arg->argv[i]);
        fmt_s(fmt, " ");
    }
    fmt_s(fmt, "[ACTION]");
    fmt_s(fmt, "\n");
    fmt_s(fmt, "\n");

    fmt_s(fmt, "Supported actions: \n");
    for (u32 i = 0; i < arg->opt_count; ++i) {
        fmt_ss(fmt, "    ", arg->opts[i][0], ": ");
        fmt_ss(fmt, "", arg->opts[i][1], "\n");
    }
}

static void arg_help_opt(Arg *arg) {
    if (arg->i >= arg->argc) return;
    arg_help(arg, fout);
    os_exit(1);
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
static void build_tmp(Arg *arg) {
    bool quick = arg_match(arg, "quick", "Skip other platforms");
    bool release = arg_match(arg, "release", "Build in release mode");
    bool run = arg_match(arg, "run", "Run tmp directly with hot reload");
    arg_help_opt(arg);

    os_system("mkdir -p out/tmp");
    Mode mode = Mode_Debug;
    if (release) mode = Mode_Release;

    char *include[] = {"core", 0};
    clang_compile(Platform_Linux, mode, include, "app/tmp.c", "out/tmp/tmp.elf");

    if (run) os_exit(os_system("out/tmp/tmp.elf"));
    if (quick) return;
    clang_compile(Platform_Windows, mode, include, "app/tmp.c", "out/tmp/tmp.exe");
    clang_compile(Platform_WASM, mode, include, "app/tmp.c", "out/tmp/tmp.wasm");

    // Generate html page
    char *js_path[] = {"core/os_wasm.js", "app/tmp.js", 0};
    char *wasm_path = "out/tmp/tmp.wasm";
    char *html_path = "app/tmp.html";
    generate_html("out/tmp/tmp.html", 0, js_path, wasm_path, html_path);
}

static void build_test(Arg *arg) {
    char *include[] = {"core", 0};
    clang_compile(Platform_Linux, Mode_Debug, include, "app/test.c", "out/test.elf");
    os_exit(os_system("out/test.elf"));
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

    if (arg_match(&arg, "tmp", "Build TMP")) {
        build_tmp(&arg);
        os_exit(0);
        return;
    }

    if (arg_match(&arg, "lsp", "Generate compile_commands.json for autocompile")) {
        generate_lsp(&arg);
        os_exit(0);
        return;
    }

    arg_help(&arg, fout);
    os_exit(1);
}
