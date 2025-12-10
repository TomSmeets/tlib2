// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// hot.h - Generic application hot reloader
#include "hot.h"
#include "fmt.h"
#include "type.h"

// Permanent memory
static Memory *mem;

// Hot-Reload state
static Hot *hot;

// File watcher state
static File *watch;

// Compile the c file in 'input_path' to an output path.
static bool hot_compile(Memory *tmp, char *input_path) {
    // Generate a output path
    char *output_path = os_mktmp(tmp, "main.so");

    // Make a compile command
    Fmt *fmt = fmt_new(tmp);
    fmt_s(fmt, "clang");
    fmt_ss(fmt, " -o ", output_path, "");
    fmt_s(fmt, " -Isrc");
    fmt_s(fmt, " -shared");
    fmt_s(fmt, " -fPIC");
    fmt_ss(fmt, " ", input_path, "");
    char *cmd = fmt_end(fmt);

    // Run command
    fmt_ss(ferr, "Cmd: ", cmd, "\n");
    i32 ret = os_system(cmd);

    // Return false on failure
    if (ret != 0) return false;

    // Load new application
    hot_load(hot, output_path);
    return true;
}

void os_main(u32 argc, char **argv) {
    bool init = !hot;

    if (init) {
        // Create a memory arena for all future allocations
        mem = mem_new();

        // Check arguments
        if (argc < 2) {
            fmt_s(ferr, "Usage: ");
            fmt_s(ferr, argv[0]);
            fmt_s(ferr, " <SOURCE> [ARGS...]\n");
            os_exit(1);
        }

        // Create a Hot-Reload helper (see hot.h)
        hot = hot_new(mem);

        // Create a file watcher that watches 'src' and 'app' for file changes
        // os_watch_check will return true each time one or more files are changed
        watch = os_watch_new();
        os_watch_add(watch, "src");
        os_watch_add(watch, "app");
    }

    // Reload applcation when a file was changed
    if (os_watch_check(watch) || init) {
        hot->os_main = 0;
        Memory *tmp = mem_new();
        hot_compile(tmp, argv[1]);
        mem_free(tmp);
    }

    if (hot->os_main) {
        // Run applcation update method
        hot->os_main(argc - 1, argv + 1);
    } else {
        // If no application was loaded, poll slowly
        os_sleep(100 * 1000);
    }
}
