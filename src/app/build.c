// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#include "build/hot.h"
#include "core/os.h"

static Memory *mem;

typedef struct {
    bool match;
    u32 argc;
    const char **argv;
} Cli;

static Cli cli_init(u32 argc, const char **argv) {
    return (Cli){.argc = argc, .argv = argv};
}

static bool cli_command(Cli *cli, const char *name) {
    if (cli->argc < 2) return false;
    if (cli->match) return false;
    if (!str_eq(cli->argv[1], name)) return false;
    cli->match = true;
    return true;
}

static Hot *hot;

static void cmd_run(Cli *cli) {
    if (!cli_command(cli, "run")) return;
    if (!hot) hot = hot_new(mem);
    hot_load(hot, "./out/main.so");
    hot_call(hot, cli->argc, cli->argv);
    hot_call(hot, cli->argc, cli->argv);
    hot_load(hot, "./out/main2.so");
    hot_call(hot, cli->argc, cli->argv);
    hot_call(hot, cli->argc, cli->argv);
    os_exit(0);
}

static void cmd_help(Cli *cli) {
    if (cli->match) return;
    os_exit(1);
}

void os_main(u32 argc, const char **argv) {
    if (!mem) mem = mem_new();

    Cli cli = cli_init(argc, argv);
    cmd_run(&cli);
    cmd_help(&cli);
}
