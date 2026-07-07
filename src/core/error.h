// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// error.h - Very simple error handling
#pragma once
#include "ansi.h"
#include "macro.h"
#include "type.h"

// Set to a message if an error occurred, null otherwise
static thread_local char *error;

// This makes error checking very simple:
// - Check error:   if (error) ...
// - Clear error:   error = 0
// - Trigger error: error = "..."
//
// The check(...) macro evaluates the expression and sets error in case of failure.
// Unlike assert it will not abort the execution.
//
// Errors should not be checked after every call, only where required.
// When an error occurs you should return a default value if this is possible.

// Throw an error
static void error_set(char *message) {
    // We are only interested in the first error that occurred
    if (!error) error = message;
}

// Throw an error if the condition becomes false
// - Returns the condition value
static bool error_check(bool cond, char *message) {
    if (!cond) error_set(message);
    return cond;
}

#if OS_LINUX
#define ERROR_MESSAGE(MSG) ANSI_BOLD __FILE__ ":" TO_STRING(__LINE__) ": " ANSI_RED "Error: " ANSI_RESET ANSI_BOLD MSG ANSI_RESET "\n"
#else
#define ERROR_MESSAGE(MSG) __FILE__ ":" TO_STRING(__LINE__) ": Error: " MSG "\n"
#endif

#define check(X) error_check((X), ERROR_MESSAGE("check(" #X ") failed"))
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
