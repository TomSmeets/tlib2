// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#include "core/os.h"
#include "hot/hot.h"



typedef struct {
    bool match;
    u32 argc;
    const char **argv;
} Cli;

static Cli cli_init(u32 argc, const char **argv) {
    return (Cli) { .argc = argc, .argv = argv };
}

static bool cli_command(Cli *cli, const char *name) {
    if(cli->argc < 2) return false;
    if(cli->match) return false;
    if(!str_eq(cli->argv[1], name)) return false;
    cli->match = true;
    return true;
}


static Hot *hot;
static void cmd_run(Cli *cli) {
    if (!cli_command(cli, "run")) return;
    if(!hot) hot = hot_new();
    hot_load(hot, "./out/main.so");
    hot->os_main(cli->argc, cli->argv);
    hot->os_main(cli->argc, cli->argv);
    hot_load(hot, "./out/main2.so");
    hot->os_main(cli->argc, cli->argv);
    hot->os_main(cli->argc, cli->argv);
    os_exit(0);
}

static void cmd_help(Cli *cli) {
    if(cli->match) return;
    printf("HELP!\n");
    os_exit(1);
}

void os_main(u32 argc, const char **argv) {
    Cli cli = cli_init(argc, argv);
    cmd_run(&cli);
    cmd_help(&cli);
}
