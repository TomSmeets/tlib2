// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// error.h - Very simple error handling
#pragma once
#include "type.h"

// Must be defined
static thread_local char *error;

// check() sets an error message if the condition fails but still continues execution
// Errors can be checked with: if(error) ...
// Errors can be cleared with: error = 0;
// All functions should return some kind of default on error
// Checking for the errors should be mostly optional
// Errors should be able to be handled at some later time

// Throw an error
static void _error_set(char *message) {
    // We are only interested in the first error that occurred
    if (!error) error = message;
}

// Throw an error if the condition becomes false
// - Returns the condition value
static bool _error_check(bool cond, char *message) {
    if (!cond) _error_set(message);
    return cond;
}

#if OS_LINUX
#define ANSI_BOLD "\e[1m"
#define ANSI_RED "\e[31m"
#define ANSI_RESET "\e[0m"
#else
#define ANSI_BOLD ""
#define ANSI_RED ""
#define ANSI_RESET ""
#endif

#define check_msg(X, MSG) \
    _error_check((X), ANSI_BOLD __FILE__ ":" TO_STRING(__LINE__) ": " ANSI_RED "Error: " ANSI_RESET ANSI_BOLD MSG ANSI_RESET "\n")
#define check(X) check_msg(X, "check(" #X ") failed")
#define check_or(X) if (!check(X))

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
