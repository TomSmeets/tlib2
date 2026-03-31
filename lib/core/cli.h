// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// cli.h - Command line argument parser
//
// This program parses the following command format.
// > ./main <COMMAND> [VALUE...] [FLAG...]
//
// Help text is generated automatically
#pragma once
#include "cli_arg.h"
#include "fmt.h"
#include "list.h"
#include "mem.h"

typedef struct Cli Cli;

// Create a new command line argument parser
static Cli *cli_new(Memory *mem, char **argv);

// Match a sub command
// - Returns true if this subcommand should be executed
static void cli_command(Cli *cli, char *command, char *info);

// Check a flag of the current cli command
static bool cli_flag(Cli *cli, char *name_short, char *name_long, char *info);

// Match any value
static char *cli_value(Cli *cli, char *name, char *info);

// Collect remaining arguments into an array
static char **cli_remaining(Cli *cli, char *argv0);

// Check if the current command was matched correctly
static bool cli_check(Cli *cli);

// Show help message if no command was matched
static void cli_help(Cli *cli);

static void test_cli(void) {
    Memory *mem = mem_new();

    Cli *cli = cli_new(mem, (char *[]){"hello", "-xy", "--world", "-z", 0});
    cli_command(cli, "test", "");
    check(cli_flag(cli, "-x", "--xx", "") == 0);
    check(cli_check(cli) == 0);

    cli_command(cli, "hello", "");
    check(cli_flag(cli, "-x", "--xx", "") == 1);
    check(cli_flag(cli, "-y", "--yy", "") == 1);
    check(cli_flag(cli, "-z", "--zz", "") == 0);
    check(cli_flag(cli, "-w", "--world", "") == 1);
    check(cli_check(cli) == 1);

    cli_command(cli, "world", "");
    check(cli_flag(cli, "-x", "--xx", "") == 0);
    check(cli_check(cli) == 0);
}

// ==== Implementation ====

// Cli flag documentation
typedef struct Cli_Flag Cli_Flag;
struct Cli_Flag {
    char *name_short;
    char *name_long;
    char *info;
    Cli_Arg *match;
    Cli_Flag *next;
};

// Cli input value documentation
typedef struct Cli_Value Cli_Value;
struct Cli_Value {
    char *name;
    char *info;
    Cli_Arg *match;
    Cli_Value *next;
};

// Cli subcommand documentation
typedef struct Cli_Command Cli_Command;
struct Cli_Command {
    char *name;
    char *info;
    Cli_Arg *match;
    Cli_Flag *flag_first, *flag_last;
    Cli_Value *value_first, *value_last;
    Cli_Command *next;
};

struct Cli {
    Memory *mem;
    char *program_name;
    Cli_Arg *argv;

    // Documentation
    Cli_Command *command_first, *command_last;
};

static Cli *cli_new(Memory *mem, char **argv) {
    // There should be at least one argument
    assert(argv[0]);

    Cli *cli = mem_struct(mem, Cli);
    cli->mem = mem;
    cli->program_name = argv[0];
    cli->argv = cli_arg_split(mem, argv + 1);
    return cli;
}

static void cli_command(Cli *cli, char *name, char *info) {
    Cli_Command *cmd = mem_struct(cli->mem, Cli_Command);
    cmd->name = name;
    cmd->info = info;
    LIST_APPEND(cli->command_first, cli->command_last, cmd);

    for (Cli_Arg *arg = cli->argv; arg; arg = arg->next) {
        if (arg->is_used) continue;
        if (arg->is_flag_long) continue;
        if (arg->is_flag_short) continue;
        if (str_eq(arg->name, name)) {
            cmd->match = arg;
            arg->is_used = 1;
        }
        break;
    }
}

static bool cli_check(Cli *cli) {
    // Return false if command dit not match
    if (!cli->command_last->match) return false;

    // Return false if not all arguments are used
    for (Cli_Arg *arg = cli->argv; arg; arg = arg->next) {
        if (!arg->is_used) return false;
    }

    // Command was valid
    return true;
}

static bool cli_flag(Cli *cli, char *name_short, char *name_long, char *info) {
    if (name_short) assert(name_short[0] == '-' && name_short[1] != '-');
    if (name_long) assert(name_long[0] == '-' && name_long[1] == '-' && name_long[2] != '-');
    Cli_Command *cmd = cli->command_last;

    Cli_Flag *doc = mem_struct(cli->mem, Cli_Flag);
    doc->name_short = name_short;
    doc->name_long = name_long;
    doc->info = info;
    LIST_APPEND(cmd->flag_first, cmd->flag_last, doc);

    // Skip check if the command is not valid
    if (!cmd->match) return 0;

    for (Cli_Arg *arg = cli->argv; arg; arg = arg->next) {
        if (arg->is_used) continue;
        bool match_short = arg->is_flag_short && str_eq(arg->name, name_short);
        bool match_long = arg->is_flag_long && str_eq(arg->name, name_long);
        if (!match_short && !match_long) continue;
        arg->is_used = true;
        doc->match = arg;
    }

    return doc->match;
}

static char *cli_value(Cli *cli, char *name, char *info) {
    Cli_Command *cmd = cli->command_last;

    Cli_Value *doc = mem_struct(cli->mem, Cli_Value);
    doc->name = name;
    doc->info = info;
    LIST_APPEND(cmd->value_first, cmd->value_last, doc);

    // Skip check if the command is not valid
    if (cmd->match) return 0;

    for (Cli_Arg *arg = cli->argv; arg; arg = arg->next) {
        if (arg->is_used) continue;
        if (arg->is_flag_long) continue;
        if (arg->is_flag_short) continue;
        arg->is_used = true;
        return arg->name;
    }

    return 0;
}

static void cli_help(Cli *cli) {
    // Find matched command
    Cli_Command *cmd = cli->command_first;
    while (cmd && !cmd->match) cmd = cmd->next;

    if (!cmd) {
        fmt_s(ferr, "Usage: ");
        fmt_s(ferr, cli->program_name);
        fmt_s(ferr, " <COMMAND> [VALUES...] [FLAGS...]\n");
        for (Cli_Command *cmd = cli->command_first; cmd; cmd = cmd->next) {
            fmt_s(ferr, "  ");
            fmt_s(ferr, cli->program_name);
            fmt_s(ferr, " ");
            fmt_s(ferr, cmd->name);
            fmt_pad_line(ferr, 20, ' ');
            fmt_s(ferr, " | ");
            fmt_s(ferr, cmd->info);
            fmt_s(ferr, "\n");
        }
        return;
    }

    bool show_help = 0;

    // Check if all arguments are used
    for (Cli_Arg *arg = cli->argv; arg; arg = arg->next) {
        if (arg->is_used) continue;
        show_help = 1;
        break;
    }

    // No problem
    if (show_help) {
        fmt(ferr, "Error:\n");
        for (Cli_Arg *arg = cli->argv; arg; arg = arg->next) {
            if (arg->is_used) continue;
            if (arg->is_flag_long || arg->is_flag_short) {
                fmt(ferr, "  Invalid option: '", arg->name, "'\n");
            } else {
                fmt(ferr, "  Invalid argument: '", arg->name, "'\n");
            }
        }
        fmt(ferr, "\n");

        fmt(ferr, "Usage: ", cli->program_name, " ", cmd->name);
        for (Cli_Value *val = cmd->value_first; val; val = val->next) {
            fmt(ferr, " ", val->name);
        }
        fmt(ferr, "\n");
        for (Cli_Value *val = cmd->value_first; val; val = val->next) {
            fmt_s(ferr, "    ");
            fmt_s(ferr, val->name);
            fmt_pad_line(ferr, 20, ' ');
            fmt_s(ferr, " | ");
            fmt_s(ferr, val->info);
            fmt_s(ferr, "\n");
        }
        for (Cli_Flag *flag = cmd->flag_first; flag; flag = flag->next) {
            fmt_s(ferr, "    ");
            if (flag->name_short) {
                fmt_s(ferr, flag->name_short);
                fmt_s(ferr, ", ");
            }
            fmt_s(ferr, flag->name_long);
            fmt_pad_line(ferr, 20, ' ');
            fmt_s(ferr, " | ");
            fmt_s(ferr, flag->info);
            fmt_s(ferr, "\n");
        }
    }
}

static char **cli_remaining(Cli *cli, char *argv0) {
    Cli_Command *cmd = cli->command_last;

    Cli_Value *doc = mem_struct(cli->mem, Cli_Value);
    doc->name = "[ARGS...]";
    doc->info = "Remaining arguments";
    LIST_APPEND(cmd->value_first, cmd->value_last, doc);

    if (!cmd->match) return 0;

    u32 count = 0;
    if (argv0) count++;
    for (Cli_Arg *arg = cli->argv; arg; arg = arg->next) {
        if (arg->is_used) continue;
        if (arg->is_flag_long) continue;
        if (arg->is_flag_short) continue;
        count++;
    }

    char **argv = mem_array(cli->mem, char *, count + 1);
    u32 i = 0;
    if (argv0) argv[i++] = argv0;
    for (Cli_Arg *arg = cli->argv; arg; arg = arg->next) {
        if (arg->is_used) continue;
        if (arg->is_flag_long) continue;
        if (arg->is_flag_short) continue;
        argv[i++] = arg->name;
        arg->is_used = 1;
    }
    assert(i == count);
    argv[count] = 0;
    return argv;
}
