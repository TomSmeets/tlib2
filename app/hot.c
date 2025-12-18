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
static Watch *watch;

static char *build_command;
static char *output_path;
static char *entry_name;

static void (*entry_method)(u32, char **);

void os_main(u32 argc, char **argv) {
    bool init = !hot;
    if (init) {
        // Check arguments
        if (argc < 4) {
            fmt_s(ferr, "Usage: ");
            fmt_s(ferr, argv[0]);
            fmt_s(ferr, " <COMMAND> <OUPTUT> <ENTRY> [WATCH...] -- [ARG...]\n");
            fmt_s(ferr, "\n");
            fmt_s(ferr, "  <COMMAND>  - The (build) command to execute on a file change.\n");
            fmt_s(ferr, "  <OUTPUT>   - The output executable created by the build command.\n");
            fmt_s(ferr, "  <ENTRY>    - Entry point symbol name, this function is just called in an infinite loop.\n");
            fmt_s(ferr, "  [WATCH...] - A list of files to watch for changes.\n");
            fmt_s(ferr, "  [ARG...]   - Arguments to be passed to the application.\n");
            fmt_s(ferr, "\n");
            fmt_s(ferr, "Example:\n");
            fmt_ss(ferr, "  ", argv[0], " 'clang -o out/main.so -shared src/main.c' out/main.so src app\n");
            fmt_ss(ferr, "  ", argv[0], " make build/application.so src -- test\n");
            os_exit(1);
        }

        build_command = argv[1];
        output_path = argv[2];
        entry_name = argv[3];

        u32 watch_count = 0;
        char **watch_paths = &argv[4];

        // Find '--' separating our from the child arguments
        for (u32 i = 4; i < argc; ++i) {
            if (str_eq(argv[i], "--")) break;
            watch_count++;
        }

        // Create a memory arena for all future allocations
        mem = mem_new();

        // Create a Hot-Reload helper (see hot.h)
        hot = hot_new(mem);

        // Create a file watcher that watches 'src' and 'app' for file changes
        // os_watch_check will return true each time one or more files are changed
        watch = os_watch_new();
        for (u32 i = 0; i < watch_count; ++i) {
            fmt_ss(ferr, "Watching: ", watch_paths[i], "\n");
            os_watch_add(watch, watch_paths[i]);
        }
    }

    // Reload applcation when a file was changed
    if (os_watch_check(watch) || init) {
        // Forget old entry point
        entry_method = 0;

        // Run build command
        i32 ret = os_system(build_command);

        // Load the new application
        if (ret == 0) {
            entry_method = hot_load(hot, output_path, entry_name);
        }
    }

    if (entry_method) {
        // Run applcation update method
        entry_method(argc - 1, argv + 1);
    } else {
        // If no application was loaded, poll slowly
        os_sleep(100 * 1000);
    }
}
