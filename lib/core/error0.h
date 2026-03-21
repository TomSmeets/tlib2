// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// error.h - Very simple error handling
#pragma once
#include "type.h"

thread_local static char *error;

#define check(X)                                                                                                                                     \
    if (!(X) && !error) error = __FILE__ ":" TO_STRING(__LINE__) ": Error check(" #X ") failed\n"
