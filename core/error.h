// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// error.h - Assert like error handling without crashing
#pragma once
#include "type.h"

// Goal: Error handling with the ease of `assert` but without crashing
static u32 error_count;
static char *error_message[16];

// Exit current function with 0 when condition becomes false
#define try(cond, ...)                                                                                                                               \
    if (!(cond)) return error_set(__FILE__ ":" TO_STRING(__LINE__) ": try(" #cond ") failed " __VA_ARGS__ " \n"), (void *)0

// Clear errors and return 1
// Usage:
//   return ok();
// or to return a value
//   return ok(), VALUE;
#define ok() error_clear(), 1

// try(x)
//  0 -> set error,   return 0
//  x -> clear error, evaluate to x

// Set an error message, and return a null-value
static void error_set(char *message) {
    if (error_count >= array_count(error_message)) return;
    error_message[error_count++] = message;
}

// Clear all current error messages, and return the input value
static void error_clear(void) {
    error_count = 0;
}
