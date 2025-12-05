// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#include "build/hot.h"
#include "core/os.h"
#include "std/cli.h"

static Memory *mem;
static Hot *hot;

static void cmd_run(Cli *cli) {
    if (!cli_command(cli, "run", "Run with hot reloading")) return;
    if (!hot) hot = hot_new(mem);
    hot_load(hot, "./out/main.so");
    hot_call(hot, cli->argc, cli->argv);
    hot_call(hot, cli->argc, cli->argv);
    hot_load(hot, "./out/main2.so");
    hot_call(hot, cli->argc, cli->argv);
    hot_call(hot, cli->argc, cli->argv);
    os_exit(0);
}

void os_main(u32 argc, const char **argv) {
    if (!mem) mem = mem_new();

    Cli *cli = cli_new(mem, argc, argv);
    cmd_run(cli);
    cli_help(cli);
}
