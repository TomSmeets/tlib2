// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// os_main.h - Main entrypoint of your application
#pragma once
#include "error.h"
#include "mem.h"
#include "os_exit.h"

// Command line arguments
static char **os_argv;

// Main entry point, called forever until an error or a call to os_exit()
static void os_main(void);

// Can be called dynamically
void os_main_wrapper(int argc, char **argv) {
    // Remember argv
    if (!os_argv && argv) os_argv = argv;

    // Call main method
    os_main();

    // Reset temporary memory
    mem_tmp_free();

    // Exit on error
    if (error) os_exit();
}

#if OS_LINUX
int main(i32 argc, char **argv) {
    for (;;) os_main_wrapper(argc, argv);
}
#elif OS_WINDOWS
int main(int argc, char **argv) {
    for (;;) os_main_wrapper(argc, argv);
}
#endif
