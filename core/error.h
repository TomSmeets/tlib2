#pragma once
#include "os.h"

static u32 error_count;
static char *error_message[64];

#define try(cond)                                                                                                                                  \
    if (!(cond)) return error_set(__FILE__ ":" TO_STRING(__LINE__) ": try(" #cond ") failed\n")
#define try_msg(cond, msg)                                                                                                                         \
    if (!(cond)) return error_set(__FILE__ ":" TO_STRING(__LINE__) ": try(" #cond ") failed, " msg "\n")

static bool error_set(char *message) {
    error_message[(error_count++) % array_count(error_message)] = message;
    return 0;
}

static bool ok(void) {
    error_count = 0;
    return 1;
}

static void error_exit(void) {
    for(u32 i = 0; i < error_count; ++i) {
        os_write(os_stderr(), error_message[i], str_len(error_message[i]), 0);
    }
    os_exit(1);
}
