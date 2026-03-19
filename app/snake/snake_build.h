// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// snake_build.h - Makefile for snake
#include "build.h"
#include "arg.h"


static bool snake_build(Arg *arg) {
    if (!arg_match(arg, "snake", "Build Snake")) return ok();
    bool quick = arg_match(arg, "quick", "Skip other platforms");
    bool release = arg_match(arg, "release", "Build in release mode");
    bool run = arg_match(arg, "run", "Run snake directly with hot reload");

    // Show help message if unknown argument was passed
    arg_help_opt(arg);

    Memory *mem = mem_new();
    Build *build = build_new(mem, "app/snake/snake.c", "snake");
    build->release = release;
    build->linux = 1;
    if (!quick && !run) {
        build->windows = 1;
        build->wasm = !release;
        build->html = 1;
    }

    build_js(build, "lib/core/os_wasm.js");
    build_js(build, "lib/pix/pix_wasm.js");
    build_js(build, "app/snake/snake.js");
    build_css(build, "app/snake/snake.css");
    build_html(build, "app/snake/snake.html");
    try(build_build(build));

    if (run) try(os_system("out/snake/snake.elf") == 0);
    os_exit(0);
    return ok();
}
