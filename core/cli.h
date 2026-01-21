// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// cli.h - Command line argument parser
//
// This program parses the following command format.
// > ./main <COMMAND> [VALUE...] [FLAG...]
//
// Help text is generated automatically
#pragma once
#include "fmt.h"
#include "list.h"
#include "mem.h"

typedef struct Cli Cli;

// Create a new command line argument parser
static Cli *cli_new(Memory *mem, u32 argc, char **argv);

// Match a sub command
static bool cli_command(Cli *cli, char *command, char *info);

// Match a sub command
static bool cli_flag(Cli *cli, char *name_short, char *name_long, char *info);

// Show help message if no command was matched
static void cli_help(Cli *cli);

// Collect remaining arguments into a new argv array
static u32 cli_get_remaining(Cli *cli, char *argv0, u32 count, char **argv);

// ==== Implementation ====
typedef struct Cli_Command Cli_Command;
typedef struct Cli_Flag Cli_Flag;
typedef struct Cli_Value Cli_Value;
typedef struct Cli_Arg Cli_Arg;

// Cli subcommand documentation
struct Cli_Command {
    char *name;
    char *info;
    Cli_Command *next;

    Cli_Flag *flags, *flags_last;
    Cli_Value *values, *values_last;
};

// Cli flag documentation
struct Cli_Flag {
    char *name_short;
    char *name_long;
    char *info;
    Cli_Flag *next;
};

// Cli input value documentation
struct Cli_Value {
    char *name;
    char *info;
    Cli_Value *next;
};

// Command line argument
struct Cli_Arg {
    char *name;
    bool is_used;
    bool is_flag;
    Cli_Arg *next;
};

struct Cli {
    bool has_match;
    char *program_name;
    Cli_Arg *args;

    // Memory used for allocating doc nodes
    Memory *mem;

    // Generated documentation
    Cli_Command *doc, *doc_last;
};

// Create a new command line argument parser
static Cli *cli_new(Memory *mem, u32 argc, char **argv) {
    Cli *cli = mem_struct(mem, Cli);
    cli->mem = mem;
    cli->program_name = argv[0];

    bool match_flags = true;
    Cli_Arg *arg_last = 0;
    for (u32 i = 1; i < argc; ++i) {
        char *arg_name = argv[i];

        if (match_flags && str_eq(arg_name, "--")) {
            match_flags = false;
            continue;
        }

        Cli_Arg *arg = mem_struct(mem, Cli_Arg);
        arg->name = arg_name;
        arg->is_flag = match_flags && arg->name[0] == '-';
        LIST_APPEND(cli->args, arg_last, arg);
    }

    Cli_Command *no_subcommand = mem_struct(mem, Cli_Command);
    cli->doc = cli->doc_last = no_subcommand;
    return cli;
}

// Find first non-flag argument (the subcommand)
static Cli_Arg *_cli_find_subcommand(Cli *cli) {
    Cli_Arg *arg = cli->args;
    while (arg && arg->is_flag) arg = arg->next;
    return arg;
}

// Match a sub command
static bool cli_command(Cli *cli, char *command, char *info) {
    // Update help text
    Cli_Command *doc = mem_struct(cli->mem, Cli_Command);
    doc->name = command;
    doc->info = info;
    LIST_APPEND(cli->doc, cli->doc_last, doc);

    // Check if the argument matches
    Cli_Arg *arg = _cli_find_subcommand(cli);
    if (!arg) return false;
    if (!str_eq(arg->name, command)) return false;

    // Match!
    arg->is_used = true;
    cli->has_match = true;
    return true;
}

static char *cli_value(Cli *cli, char *name, char *info) {
    Cli_Command *cmd = cli->doc_last;
    Cli_Value *doc = mem_struct(cli->mem, Cli_Value);
    doc->name = name;
    doc->info = info;
    LIST_APPEND(cmd->values, cmd->values_last, doc);

    for (Cli_Arg *arg = cli->args; arg; arg = arg->next) {
        if (arg->is_used) continue;
        if (arg->is_flag) continue;
        arg->is_used = true;
        return arg->name;
    }

    return 0;
}

static bool cli_flag(Cli *cli, char *name_short, char *name_long, char *info) {
    Cli_Command *cmd = cli->doc_last;
    Cli_Flag *doc = mem_struct(cli->mem, Cli_Flag);
    doc->name_short = name_short;
    doc->name_long = name_long;
    doc->info = info;
    LIST_APPEND(cmd->flags, cmd->flags_last, doc);

    for (Cli_Arg *arg = cli->args; arg; arg = arg->next) {
        if (arg->is_used) continue;
        if (!arg->is_flag) continue;
        bool match_short = str_eq(arg->name, name_short);
        bool match_long = str_eq(arg->name, name_long);
        if (!match_short && !match_long) continue;
        arg->is_used = true;
        return true;
    }

    return false;
}

static void cli_cmdhelp(Cli *cli) {
    Cli_Command *cmd = cli->doc_last;
    fmt_s(ferr, "Usage: ");
    fmt_s(ferr, cli->program_name);
    fmt_s(ferr, " ");
    fmt_s(ferr, cmd->name);
    for (Cli_Value *val = cmd->values; val; val = val->next) {
        fmt_s(ferr, " ");
        fmt_s(ferr, val->name);
    }
    fmt_s(ferr, "\n");
    for (Cli_Value *val = cmd->values; val; val = val->next) {
        fmt_s(ferr, "    ");
        fmt_s(ferr, val->name);
        fmt_s(ferr, " | ");
        fmt_s(ferr, val->info);
        fmt_s(ferr, "\n");
    }
    for (Cli_Flag *flag = cmd->flags; flag; flag = flag->next) {
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
    os_exit(1);
}

static void cli_help(Cli *cli) {
    if (cli->has_match) return;
    fmt_s(ferr, "Usage: ");
    fmt_s(ferr, cli->program_name);
    fmt_s(ferr, " <COMMAND> [VALUES...] [FLAGS...]\n");
    for (Cli_Command *cmd = cli->doc->next; cmd; cmd = cmd->next) {
        fmt_s(ferr, "  ");
        fmt_s(ferr, cli->program_name);
        fmt_s(ferr, " ");
        fmt_s(ferr, cmd->name);
        fmt_pad_line(ferr, 20, ' ');
        fmt_s(ferr, " | ");
        fmt_s(ferr, cmd->info);
        fmt_s(ferr, "\n");
    }
    os_exit(1);
}

static u32 cli_get_remaining(Cli *cli, char *argv0, u32 count, char **argv) {
    u32 i = 0;
    if (argv0 && count > 0) argv[i++] = argv0;
    for (Cli_Arg *arg = cli->args; arg; arg = arg->next) {
        if (i >= count) break;
        if (arg->is_used) continue;
        argv[i++] = arg->name;
    }
    return i;
}
