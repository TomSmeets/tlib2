// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#include "core/os.h"
#include "hot/hot.h"

static Hot *hot;

void os_main(u32 argc, const char **argv) {
    if (argc < 3) os_exit(1);
    const char *command = argv[1];
    const char *source = argv[2];
    if (!str_eq(command, "run")) os_exit(1);

    hot = hot_new();
    hot_load(hot, "./out/main.so");
    hot->os_main(argc, argv);
    hot->os_main(argc, argv);
    hot_load(hot, "./out/main2.so");
    hot->os_main(argc, argv);
    hot->os_main(argc, argv);
    os_exit(0);
}
