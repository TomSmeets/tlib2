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

#if OS_LINUX
#define ANSI_BOLD "\e[1m"
#define ANSI_RED "\e[31m"
#define ANSI_RESET "\e[0m"
#else
#define ANSI_BOLD ""
#define ANSI_RED ""
#define ANSI_RESET ""
#endif

#define check_msg(X, MSG) error_set((X), ANSI_BOLD __FILE__ ":" TO_STRING(__LINE__) ": " ANSI_RED "Error: " ANSI_RESET ANSI_BOLD MSG ANSI_RESET "\n")

#define check(X) check_msg(X, "check(" #X ") failed")
#define check_or(X) if (check_msg(X, "check_or(" #X ") failed"))
#define assert(X)                                                                                                                                    \
    if (check_msg(X, "assert(" #X ") failed")) os_exit()
#define os_fail(MSG) check_msg(0, MSG), os_exit()

// Set the error flag if the condition becomes false
static bool error_set(bool cond, char *message) {
    // If condition is true then there is no error
    if (cond) return 0;

    // Set the error message only if this is the first error
    if (!error) error = message;

    // Return true if the condition fails
    return 1;
}

// Clear error flag (ignores the last error)
static void error_clear(void) {
    error = 0;
}

// Return the error flag and clear it
static char *error_pop(void) {
    char *ret = error;
    error_clear();
    return ret;
}
