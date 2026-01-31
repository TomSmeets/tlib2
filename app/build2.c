#include "fmt.h"

typedef enum {
    Action_Build,
    Action_Format,
    Action_Test,
    Action_Count,
} Action;

static char *action_str[] = {
    "build",
    "format",
    "test",
};

typedef enum {
    Platform_Cross,
    Platform_Linux,
    Platform_Windows,
    Platform_WASM,
    Platform_Count,
} Platform;

static char *platform_str[] = {
    "cross",
    "linux",
    "windows",
    "wasm",
};

typedef enum {
    Mode_Debug,
    Mode_Release,
    Mode_Count,
} Mode;

static char *mode_str[] = {
    "debug",
    "release",
};

typedef enum {
    App_All,
    App_Snake,
    App_Count,
} App;

typedef struct {
    Action action;
    Platform platform;
    Mode mode;
} Args;

static bool args_parse(Args *args, char *arg) {
    for (Action i = 0; i < Action_Count; ++i) {
        if (str_eq(arg, action_str[i])) {
            args->action = i;
            return 1;
        }
    }

    for (Platform i = 0; i < Platform_Count; ++i) {
        if (str_eq(arg, platform_str[i])) {
            args->platform = i;
            return 1;
        }
    }

    for (Mode i = 0; i < Mode_Count; ++i) {
        if (str_eq(arg, mode_str[i])) {
            args->mode = i;
            return 1;
        }
    }
    return 0;
}

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

static void clang_compile(Platform platform, Mode mode, char **include, char *input, char *output) {
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
    for(u32 i = 0; include[i]; ++i) {
        cmd_arg2(&cmd, "-I", include[i]);
    }
    cmd_arg(&cmd, input);

    fmt_cmd(fout, &cmd);
    fmt_s(fout, "\n");

    i32 ret = os_wait(os_exec(cmd.argv));
    if(ret != 0) os_exit(ret);
}


static bool os_write_str(File *file, char *data) {
    return os_write(file, data, str_len(data), 0);
}

static void fmt_file_contents(Fmt *fmt, char *input_path) {
    File *input = os_open(input_path, FileMode_Read);
    size_t used = 0;
    u8 buffer[4*1024];
    for(;;) {
        assert(os_read(input, buffer, sizeof(buffer), &used));
        if(used == 0) break;
        fmt_buf(fmt, buffer, used);
    }
    os_close(input);
}

static void fmt_base64(Fmt *fmt, u8 *data, size_t size) {
    // TODO: Fix
    fmt_s(fmt, "new Uint8Array([");
    for(u32 i = 0; i < size; ++i) {
        fmt_u(fmt, data[i]);
        fmt_s(fmt, ", ");
    }
    fmt_s(fmt, "])");
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

static void generate_html(char *output_path, char *css_path, char **js_path_list, char *wasm_path, char *html_path) {
    u8 buffer[1024*4];
    Fmt f = fmt_from(buffer, sizeof(buffer));
    f.file = os_open(output_path, FileMode_Create);

    fmt_s(&f, "<!DOCTYPE html>\n");
    fmt_s(&f, "<head>\n");
    if(css_path) {
        fmt_s(&f, "<style>\n");
        fmt_file_contents(&f, css_path);
        fmt_s(&f, "</style>\n");
    }

    fmt_s(&f, "<script>\n");
    for(u32 i = 0; js_path_list[i]; ++i) {
        fmt_file_contents(&f, js_path_list[i]);
    }
    fmt_s(&f, "tlib.main(");
    Memory *mem = mem_new();
    Buffer buf = os_read_file(mem, wasm_path);
    fmt_base64(&f, buf.data, buf.size);
    mem_free(mem);
    fmt_s(&f, ");\n");
    fmt_s(&f, "</script>\n");

    fmt_s(&f, "</head>\n");
    fmt_s(&f, "<body>\n");
    fmt_file_contents(&f, html_path);
    fmt_s(&f, "</body>\n");
    fmt_end(&f);
    os_close(f.file);
}

static void build_snake(Mode mode) {
    char *include[] = {"core", "gfx",0};
    clang_compile(Platform_Linux,   mode, include, "snake/snake.c", "out/snake/snake.elf");
    clang_compile(Platform_Windows, mode, include, "snake/snake.c", "out/snake/snake.exe");
    clang_compile(Platform_WASM,    mode, include, "snake/snake.c", "out/snake/snake.wasm");

    // Generate html page
    char *js_path[] = {"core/os_wasm.js", "gfx/pix_wasm.js", 0};
    char *css_path = "snake/snake.css";
    char *wasm_path = "out/snake/snake.wasm";
    char *html_path = "snake/snake.html";
    generate_html("out/snake/snake.html", css_path, js_path, wasm_path, html_path);
}

void os_main(u32 argc, char **argv) {
    Args args = {
        .action = Action_Build,
        .platform = Platform_Linux,
        .mode = Mode_Debug,
    };

    bool fail = 0;
    for (u32 i = 1; i < argc; ++i) {
        char *arg = argv[i];
        if (args_parse(&args, arg)) continue;
        fmt_ss(fout, " Unkown argument: ", arg, "\n");
        fail = 1;
    }

    if (fail) os_exit(1);

    fmt_s(fout, "Command:\n");
    fmt_ss(fout, "  Action:   ", action_str[args.action], "\n");
    fmt_ss(fout, "  Mode:     ", mode_str[args.mode], "\n");
    fmt_ss(fout, "  Platform: ", platform_str[args.platform], "\n");

    if (args.action == Action_Format) {
        os_exit(os_system("clang-format -i --verbose */*.h */*.c"));
        return;
    }

    // Memory *mem = mem_new();
    if (args.action == Action_Build) {
        build_snake(args.mode);
        // os_exit(clang_compile(args.platform, args.mode, "snake/snake.c", "out/snake/snake"));
    }
    os_exit(0);
}
