// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// command.h - Constructor for a unix exec call
#pragma once
#include "fmt.h"
#include "type.h"

typedef struct {
    u32 argc;
    char *argv[256];
} Command;

// Append an argument
static void cmd_arg(Command *cmd, char *arg) {
    assert(cmd->argc + 1 < array_count(cmd->argv));
    cmd->argv[cmd->argc++] = arg;
    cmd->argv[cmd->argc] = 0;
}

//  Write two arguments at the same time
static void cmd_arg2(Command *cmd, char *arg1, char *arg2) {
    cmd_arg(cmd, arg1);
    cmd_arg(cmd, arg2);
}

// Format command
static void fmt_cmd(Fmt *fmt, Command *cmd) {
    for (u32 i = 0; i < cmd->argc; ++i) {
        if (i > 0) fmt_s(fout, " ");
        fmt_s(fmt, cmd->argv[i]);
    }
}
