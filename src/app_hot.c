#include "core/os.h"
#include "core/type.h"
#include "core/fmt.h"
#include "hot/hot.h"

static Memory *mem;
static Memory *tmp;
static u32 counter = 0;
static Hot *hot;

static i32 hot_compile(const char *input_path, const char *output_path) {
    Fmt *fmt = fmt_new(tmp);
    fmt_s(fmt, "clang");
    fmt_ss(fmt, " -o ", output_path, "");
    fmt_s(fmt, " -Isrc");
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

        if(argc < 2) {
            fmt_s(stderr, "Usage: ");
            fmt_s(stderr, argv[0]);
            fmt_s(stderr, " <SOURCE> [ARGS...]\n");
            os_exit(1);
        }

        const char *source_file = argv[1];
        fmt_ss(stderr, "Source: ", source_file, "\n");

        i32 ret = hot_compile(source_file, "/tmp/main.so");
    }

    fmt_s(stdout, "Counter: ");
    fmt_s(stdout, "1234");
    fmt_s(stdout, "\n");

    mem_free(tmp);
    os_sleep(1ULL * 1000 * 1000);
}
