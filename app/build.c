// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// build.c - Optional build tool for tlib
#include "buf.h"
#include "cli.h"
#include "dwarf.h"
#include "hot.h"
#include "os.h"
#include "parse.h"
#include "tlib.h"

static Memory *mem;
static Hot *hot;

// Generate a table
// file | info | deps
static void cmd_info(Cli *cli) {
    if (!cli_command(cli, "info", "Show information about a c file")) return;

    TLib *lib = tlib_new(mem);
    for (;;) {
        char *path = cli_value(cli, "<FILE>", "Input File");
        if (!path) break;
        module_parse(lib, path);
    }

    mod_expand_links(lib);
    mod_sort(lib);

    for (Module *mod = lib->modules; mod; mod = mod->next) {
        fmt_s(fout, mod->name);
        fmt_pad_line(fout, 20, ' ');
        fmt_s(fout, " | ");
        fmt_s(fout, mod->info);
        fmt_pad_line(fout, 70, ' ');
        fmt_s(fout, " | ");

        for (Module_Link *link = mod->deps; link; link = link->next) {
            fmt_s(fout, link->module->name);
            if (link->next) fmt_s(fout, ", ");
        }
        fmt_s(fout, "\n");
    }
    os_exit(0);
}



static void cmd_run(Cli *cli) {
    static bool init;
    static Hot *hot;
    static void (*child_main)(u32, char **);
    static File *watch;
    static char *build_command;

    static u32 child_argc;
    static char *child_argv[64];

    char *output_path = "out/hot.so";
    if (!cli_command(cli, "run", "Run with hot reloading")) return;
    char *input_path = cli_value(cli, "FILE", "Main source file");

    if (!init) {
        hot = hot_new(mem);

        watch = os_watch_new();
        os_watch_add(watch, "src");
        os_watch_add(watch, "app");

        Fmt *f = fmt_new(mem);
        fmt_s(f, "clang");
        fmt_ss(f, " -o ", output_path, "");
        fmt_s(f, " -Isrc");
        fmt_s(f, " -shared");
        fmt_s(f, " -fPIC");
        fmt_s(f, " ");
        fmt_ss(f, " ", input_path, "");
        build_command = fmt_end(f);

        child_argc = cli_get_remaining(cli, input_path, array_count(child_argv), (char **)child_argv);
    }

    if(os_watch_check(watch) || !init) {
        child_main = 0;
        i32 ret = os_system(build_command);
        if (ret == 0) child_main = hot_load(hot, output_path, "os_main");
    }

    if (child_main) {
        child_main(child_argc, child_argv);
    } else {
        os_sleep(50ULL * 1000);
    }

    init = 1;
}

static void cmd_dwarf(Cli *cli) {
    if (!cli_command(cli, "dwarf", "Read dwarf info")) return;
    char *path = cli_value(cli, "<FILE>", "Input file");

    File *file = os_open(path, Open_Read);
    Elf *elf = elf_load(mem, file);
    dwarf_load(mem, elf, file);
    os_exit(0);
}

void os_main(u32 argc, char **argv) {
    if (!mem) mem = mem_new();

    Cli *cli = cli_new(mem, argc, argv);
    cmd_run(cli);
    cmd_info(cli);
    cmd_dwarf(cli);
    cli_help(cli);
}
