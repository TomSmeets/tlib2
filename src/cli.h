// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// cli.h - Command line argument parser
#pragma once
#include "core/mem.h"
#include "std/fmt.h"

typedef struct Cli Cli;

// Create a new command line argument parser
static Cli *cli_new(Memory *mem, u32 argc, const char **argv);

// Match a sub command
static bool cli_command(Cli *cli, const char *command, const char *info);

// Match a sub command
static bool cli_flag(Cli *cli, const char *flag);

// Show help message if no command was matched
static void cli_help(Cli *cli);

// ==== Implementation ====

typedef struct Cli_Command Cli_Command;
typedef struct Cli_Flag Cli_Flag;
typedef struct Cli_Value Cli_Value;

// Command
struct Cli_Command {
    const char *name;
    const char *info;
    Cli_Command *next;
    Cli_Flag  *flags;
    Cli_Value *values;
};

struct Cli_Flag {
    const char *name;
    const char *info;
    Cli_Flag *next;
};

struct Cli_Value {
    const char *name;
    const char *info;
    Cli_Value *next;
};
 
struct Cli {
    bool has_match;
    u32 argc;
    const char **argv;

    Memory *mem;
    Cli_Command *doc_start;
    Cli_Command *doc_end;
};

// Create a new command line argument parser
static Cli *cli_new(Memory *mem, u32 argc, const char **argv) {
    Cli *cli = mem_struct(mem, Cli);
    cli->argc = argc;
    cli->argv = argv;
    cli->mem = mem;
    return cli;
}

// Match a sub command
static bool cli_command(Cli *cli, const char *command, const char *info) {
    Cli_Command *doc = mem_struct(cli->mem, Cli_Command);
    doc->name = command;
    doc->info = info;
    LIST_APPEND(cli->doc_start, cli->doc_end, doc);

    if (cli->argc < 2) return false;
    if (!str_eq(cli->argv[1], command)) return false;
    cli->has_match = true;
    return true;
}

static void cli_help(Cli *cli) {
    if(cli->has_match) return;

    fmt_s(fmterr, "Usage:\n");

    u32 pad = 8;
    for(Cli_Command *cmd = cli->doc_start; cmd; cmd = cmd->next) {
        u32 len = str_len(cmd->name);
        if (len > pad) pad = len;
    }
    
    for(Cli_Command *cmd = cli->doc_start; cmd; cmd = cmd->next) {
        fmt_s(fmterr, "  ");
        fmt_s(fmterr, cli->argv[0]);
        fmt_s(fmterr, " ");
        fmt_s(fmterr, cmd->name);

        for(u32 i = str_len(cmd->name); i < pad; ++i) {
            fmt_c(fmterr, ' ');
        }

        fmt_s(fmterr, " | ");
        fmt_s(fmterr, cmd->info);
        fmt_s(fmterr, "\n");
    }
    os_exit(1);
}
