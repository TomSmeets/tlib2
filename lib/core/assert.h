// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// assert.h - Very simple error handling
#pragma once
#include "error.h"
#include "os_exit.h"

#define assert(X) \
    if (!check_msg(X, "assert(" #X ") failed")) os_exit()
#define os_fail(MSG) check_msg(0, MSG), os_exit()
