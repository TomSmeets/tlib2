// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#include "buf.h"
#include "cli.h"
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

    for(Module *mod = lib->modules; mod; mod = mod->next) {
        fmt_s(fout, mod->name);
        fmt_pad_line(fout, 10, ' ');
        fmt_s(fout, " | ");
        fmt_s(fout, mod->info);
        fmt_pad_line(fout, 60, ' ');
        fmt_s(fout, " | ");

        for(Module_Link *link = mod->deps; link; link = link->next) {
            fmt_s(fout, link->module->name);
            if (link->next) fmt_s(fout, ", ");
        }
        fmt_s(fout, "\n");
    }
    os_exit(0);
}

static void cmd_run(Cli *cli) {
    if (!cli_command(cli, "run", "Run with hot reloading")) return;
    if (!hot) hot = hot_new(mem);
    hot_load(hot, "./out/main.so");

    char *argv[64] = {};
    u32 argc = cli_get_remaining(cli, array_count(argv), (char **)argv);
    hot_call(hot, argc, argv);
    hot_call(hot, argc, argv);
    hot_load(hot, "./out/main2.so");
    hot_call(hot, argc, argv);
    hot_call(hot, argc, argv);
    os_exit(0);
}

void os_main(u32 argc, char **argv) {
    if (!mem) mem = mem_new();

    Cli *cli = cli_new(mem, argc, argv);
    cmd_run(cli);
    cmd_info(cli);
    cli_help(cli);
}
