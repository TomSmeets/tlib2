// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// os_main.h - Main entrypoint of your application
#pragma once
#include "error.h"
#include "mem.h"
#include "os_exit.h"

// Frame index
static bool os_init;

// Command line arguments
static u32 os_argc;
static char **os_argv;

// Memory arenas
static Memory *os_perm;
static Memory *os_temp;

// Main entry point, called forever until an error or a call to os_exit()
static void os_main(void);

// Can be called dynamically
void os_main_wrapper(int argc, char **argv) {
    if (!os_init) {
        os_perm = mem_new();
        os_argc = argc;
        os_argv = argv;
    }

    // Temporary frame memory
    if (os_temp) mem_free(os_temp);
    os_temp = mem_new();

    // Call main method
    os_main();

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
