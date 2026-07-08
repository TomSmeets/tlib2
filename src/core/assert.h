// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// assert.h - Very simple error handling
#pragma once
#include "error.h"
#include "os_exit.h"

// Check condition or crash
#define assert(X) check_or(X) os_exit()
#define os_fail(MSG) check(!MSG), os_exit()
#define unreachable() os_fail("Unreachable");
