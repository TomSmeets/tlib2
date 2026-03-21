#pragma once
#include "type.h"

static char *error;
#define check(X)                                                                                                                                     \
    if (!(X) && !error) error = __FILE__ ":" TO_STRING(__LINE__) ": Error check(" #X ") failed"
