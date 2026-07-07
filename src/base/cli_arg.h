#pragma once
#include "list.h"
#include "mem.h"

// Tokenized command line argument or option
typedef struct Cli_Arg Cli_Arg;
struct Cli_Arg {
    char *name;
    bool is_used;
    bool is_flag_short;
    bool is_flag_long;
    Cli_Arg *next;
};

static Cli_Arg *cli_arg_split(Memory *mem, char **argv) {
    Cli_Arg *arg_first = 0;
    Cli_Arg *arg_last = 0;

    bool check_flags = 1;
    while (1) {
        char *name = *argv++;
        if (!name) break;

        u32 name_len = str_len(name);

        bool is_short = check_flags && name_len > 1 && name[0] == '-' && name[1] != '-'; // short: -x
        bool is_long = check_flags && name_len > 2 && name[0] == '-' && name[1] == '-';  // long: --xyz
        bool is_skip = check_flags && name_len == 2 && name[0] == '-' && name[1] == '-'; // no more flags: '--'
        if (is_skip) check_flags = 0;

        // Split combined short arguments: '-xyz' becomes '-x' '-y' '-z'
        if (is_short && name_len > 2) {
            // Iterate over chars
            for (u32 i = 1; i < name_len; ++i) {
                char *short_name = mem_alloc_uninit(mem, 3);
                short_name[0] = '-';
                short_name[1] = name[i];
                short_name[2] = 0;

                Cli_Arg *arg = mem_struct(mem, Cli_Arg);
                arg->name = short_name;
                arg->is_flag_short = 1;
                LIST_APPEND(arg_first, arg_last, arg);
            }
        } else {
            // Normal argument handling
            Cli_Arg *arg = mem_struct(mem, Cli_Arg);
            arg->name = name;
            arg->is_flag_short = is_short;
            arg->is_flag_long = is_long;
            arg->is_used = is_skip;
            LIST_APPEND(arg_first, arg_last, arg);
        }
    }

    return arg_first;
}

static void test_cli_arg(void) {
    Memory *mem = mem_new();
    Cli_Arg *arg_list = cli_arg_split(mem, (char *[]){"hello", "-", "world", "-x", "-yzw", "--test", "t", "--", "-z", 0});

    {
        Cli_Arg *arg = arg_list;
        if (check(arg)) {
            check(str_eq(arg->name, "hello"));
            check(arg->is_flag_short == 0);
            check(arg->is_flag_long == 0);
            check(arg->is_used == 0);
            arg = arg->next;
        }

        if (check(arg)) {
            check(str_eq(arg->name, "-"));
            check(arg->is_flag_short == 0);
            check(arg->is_flag_long == 0);
            check(arg->is_used == 0);
            arg = arg->next;
        }

        if (check(arg)) {
            check(str_eq(arg->name, "world"));
            check(arg->is_flag_short == 0);
            check(arg->is_flag_long == 0);
            check(arg->is_used == 0);
            arg = arg->next;
        }

        if (check(arg)) {
            check(str_eq(arg->name, "-x"));
            check(arg->is_flag_short == 1);
            check(arg->is_flag_long == 0);
            check(arg->is_used == 0);
            arg = arg->next;
        }

        if (check(arg)) {
            check(str_eq(arg->name, "-y"));
            check(arg->is_flag_short == 1);
            check(arg->is_flag_long == 0);
            check(arg->is_used == 0);
            arg = arg->next;
        }

        if (check(arg)) {
            check(str_eq(arg->name, "-z"));
            check(arg->is_flag_short == 1);
            check(arg->is_flag_long == 0);
            check(arg->is_used == 0);
            arg = arg->next;
        }

        if (check(arg)) {
            check(str_eq(arg->name, "-w"));
            check(arg->is_flag_short == 1);
            check(arg->is_flag_long == 0);
            check(arg->is_used == 0);
            arg = arg->next;
        }

        if (check(arg)) {
            check(str_eq(arg->name, "--test"));
            check(arg->is_flag_short == 0);
            check(arg->is_flag_long == 1);
            check(arg->is_used == 0);
            arg = arg->next;
        }

        if (check(arg)) {
            check(str_eq(arg->name, "t"));
            check(arg->is_flag_short == 0);
            check(arg->is_flag_long == 0);
            check(arg->is_used == 0);
            arg = arg->next;
        }
        if (check(arg)) {
            check(str_eq(arg->name, "--"));
            check(arg->is_flag_short == 0);
            check(arg->is_flag_long == 0);
            check(arg->is_used == 1);
            arg = arg->next;
        }

        if (check(arg)) {
            check(str_eq(arg->name, "-z"));
            check(arg->is_flag_short == 0);
            check(arg->is_flag_long == 0);
            check(arg->is_used == 0);
            arg = arg->next;
        }

        // That should have been the last value
        check(arg == 0);
    }
    mem_free(mem);
}
