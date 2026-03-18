// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// snake_build.h - Makefile for snake
#include "build.h"
#include "arg.h"

static void snake_build(Arg *arg) {
    char *src = "app/snake";
    char *dst = "out/snake";
    // TODO

    bool quick = arg_match(arg, "quick", "Skip other platforms");
    bool release = arg_match(arg, "release", "Build in release mode");
    bool run = arg_match(arg, "run", "Run snake directly with hot reload");
    arg_help_opt(arg);

    os_system("mkdir -p out/snake");
    Build_Mode mode = Mode_Debug;
    if (release) mode = Mode_Release;

    build_compile(Platform_Linux, mode, "snake/snake.c", "out/snake/snake.elf");

    if (run) os_exit(os_system("out/snake/snake.elf"));
    if (quick) return;
    build_compile(Platform_Windows, mode, "snake/snake.c", "out/snake/snake.exe");
    build_compile(Platform_WASM, mode, "snake/snake.c", "out/snake/snake.wasm");

    // Generate html page
    char *js_path[] = {"src/os_wasm.js", "src/pix_wasm.js", "snake/snake.js", 0};
    char *css_path = "snake/snake.css";
    char *wasm_path = "out/snake/snake.wasm";
    char *html_path = "snake/snake.html";
    generate_html("out/snake/snake.html", css_path, js_path, wasm_path, html_path);

    // Cleanup
    if (release) os_remove("out/snake/snake.wasm");
}
