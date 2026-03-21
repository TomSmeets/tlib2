// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// error.h - Very simple error handling
#pragma once
#include "type.h"

// Must be defined
static void os_exit(void) __attribute__((__noreturn__));

static thread_local char *error;

// check() sets an error message if the condition fails but still continues execution
// Errors can be checked with: if(error) ...
// Errors can be cleared with: error = 0;
// All functions should return some kind of default on error
// Checking for the errors should be mostly optional
// Errors should be able to be handled at some later time
#define check_msg(X, MSG) error_set((X), __FILE__ ":" TO_STRING(__LINE__) ": " MSG " failed\n")

#define check(X) check_msg(X, "check(" #X ")")
#define check_or(X) if (check_msg(X, "check_or(" #X ")"))
#define assert(X)                                                                                                                                    \
    if (check_msg(X, "assert(" #X ")")) os_exit()
#define os_fail(MSG) check_msg(0, MSG), os_exit()

static bool error_set(bool cond, char *message) {
    if (cond) return 0;
    if (!error) error = message;
    return 1;
}
