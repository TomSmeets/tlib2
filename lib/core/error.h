// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// error.h - Very simple error handling
#pragma once
#include "type.h"

thread_local static char *error;

// check() sets an error message if the condition fails but still continues execution
// Errors can be checked with: if(error) ...
// Errors can be cleared with: error = 0;
// All functions should return some kind of default on error
// Checking for the errors should be mostly optional
// Errors should be able to be handled at some later time
#define check(X) error_set((X), __FILE__ ":" TO_STRING(__LINE__) ": Error check(" #X ") failed\n")
#define check_or(X) if (check(X))

static bool error_set(bool cond, char *message) {
    if (error) return 1;
    if (cond) return 0;
    error = message;
    return 1;
}
