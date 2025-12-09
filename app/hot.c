#include "hot.h"
#include "fmt.h"
#include "os.h"
#include "type.h"

static Memory *mem;
static Memory *tmp;
static u32 counter = 0;

static Hot *hot;
static File *watch;

static i32 hot_compile(char *input_path, char *output_path) {
    Fmt *fmt = fmt_new(tmp);
    fmt_s(fmt, "clang");
    fmt_ss(fmt, " -o ", output_path, "");
    fmt_s(fmt, " -Isrc");
    fmt_s(fmt, " -shared");
    fmt_s(fmt, " -fPIC");
    fmt_ss(fmt, " ", input_path, "");
    char *cmd = fmt_end(fmt);
    fmt_ss(ferr, "Cmd: ", cmd, "\n");
    return os_system(cmd);
}

void os_main(u32 argc, char **argv) {
    bool do_init = false;
    tmp = mem_new();

    if (!hot) {
        do_init = true;
        mem = mem_new();
        hot = hot_new(mem);
        watch = os_watch_new();
        os_watch_add(watch, "src");
        os_watch_add(watch, "app");

        if (argc < 2) {
            fmt_s(ferr, "Usage: ");
            fmt_s(ferr, argv[0]);
            fmt_s(ferr, " <SOURCE> [ARGS...]\n");
            os_exit(1);
        }
    }

    if (os_watch_check(watch) || do_init) {
        hot->os_main = 0;
        char *source_file = argv[1];
        fmt_ss(ferr, "Source: ", source_file, "\n");
        char *output_path = hot_mktmp(hot, "/tmp/main");
        i32 ret = hot_compile(source_file, output_path);
        if (ret == 0) hot_load(hot, output_path);
    }

    if (hot->os_main) {
        hot->os_main(argc - 1, argv + 1);
    } else {
        os_sleep(100 * 1000);
    }
    mem_free(tmp);
}
