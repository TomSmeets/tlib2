#include "core/fmt.h"
#include "core/os.h"
#include "core/type.h"
#include "hot/hot.h"

static Memory *mem;
static Memory *tmp;
static u32 counter = 0;
static Hot *hot;

static char *hot_mktmp(const char *prefix) {
    u64 time = os_time();
    Fmt *fmt = fmt_new(tmp);
    fmt_s(fmt, prefix);
    fmt_s(fmt, "_");
    fmt_u_ex(fmt, os_time(), 16, 0, 0);
    fmt_s(fmt, ".so");
    return fmt_end(fmt);
}

static i32 hot_compile(const char *input_path, const char *output_path) {
    Fmt *fmt = fmt_new(tmp);
    fmt_s(fmt, "clang");
    fmt_ss(fmt, " -o ", output_path, "");
    fmt_s(fmt, " -Isrc");
    fmt_s(fmt, " -shared");
    fmt_s(fmt, " -fPIC");
    fmt_ss(fmt, " ", input_path, "");
    char *cmd = fmt_end(fmt);
    fmt_ss(stdout, "Cmd: ", cmd, "\n");
    return os_system(cmd);
}

void os_main(u32 argc, const char **argv) {
    tmp = mem_new();

    if (!hot) {
        mem = mem_new();
        hot = hot_new(mem);

        if (argc < 2) {
            fmt_s(stderr, "Usage: ");
            fmt_s(stderr, argv[0]);
            fmt_s(stderr, " <SOURCE> [ARGS...]\n");
            os_exit(1);
        }
    }
    const char *source_file = argv[1];
    fmt_ss(stderr, "Source: ", source_file, "\n");
    char *output_path = hot_mktmp("/tmp/main");
    i32 ret = hot_compile(source_file, output_path);
    hot_load(hot, output_path);
    hot_call(hot, argc - 1, argv + 1);
    mem_free(tmp);
}
