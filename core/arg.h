// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// arg.h - Basic command line argument parser
#pragma once
#include "fmt.h"
#include "str.h"
#include "type.h"

typedef struct {
    // Arguments
    u32 arg_count;
    char **arg_list;

    // Current parsing index
    u32 arg_ix;

    // A list of (name, description) pairs for command line options
    // These are the options at the current argument 'index'
    u32 opt_count;
    char *opt_list[64][2];
} Arg;

// Create a new argument parser based on unix style argv and argc
static Arg arg_new(u32 argc, char **argv) {
    return (Arg){
        .arg_count = argc,
        .arg_list = argv,
        .arg_ix = 1,
    };
}

static bool arg_match(Arg *arg, char *name, char *info) {
    // Add documentation
    if (arg->opt_count < array_count(arg->opt_list)) {
        arg->opt_list[arg->opt_count][0] = name;
        arg->opt_list[arg->opt_count][1] = info;
        arg->opt_count++;
    }

    // Check if there exists an argument to match
    if (arg->arg_ix >= arg->arg_count) return false;

    // Check if argument matches
    if (!str_eq(arg->arg_list[arg->arg_ix], name)) return false;

    // Consume the argument, and advance to the next subcommand level
    arg->arg_ix++;

    // Clear all documentation from the previous sub-command level
    arg->opt_count = 0;
    return true;
}

// Write a help message with possible options
static void arg_help(Arg *arg, Fmt *fmt) {
    fmt_s(fmt, "Usage: ");
    for (u32 i = 0; i < arg->arg_ix; ++i) {
        fmt_s(fmt, arg->arg_list[i]);
        fmt_s(fmt, " ");
    }
    fmt_s(fmt, "[ACTION]");
    fmt_s(fmt, "\n");
    fmt_s(fmt, "\n");

    fmt_s(fmt, "Supported actions: \n");
    for (u32 i = 0; i < arg->opt_count; ++i) {
        fmt_ss(fmt, "    ", arg->opt_list[i][0], ": ");
        fmt_ss(fmt, "", arg->opt_list[i][1], "\n");
    }
}

// Show help message and exit if an argument was passed that was not matched
static void arg_help_opt(Arg *arg) {
    if (arg->arg_ix >= arg->arg_count) return;
    arg_help(arg, fout);
    os_exit(1);
}

static void arg_test(void) {
    char *argv[] = { "main", "test", "xyz" };
    Arg arg = arg_new(array_count(argv), argv);
    assert(arg_match(&arg, "hello", "Show Hello Message") == false);
    assert(arg_match(&arg, "world", "Something with the world") == false);
    assert(arg_match(&arg, "test", "Testing") == true);
    assert(arg_match(&arg, "xyz", "Testing") == true);
}
