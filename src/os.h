// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os.h - Operating System API
#pragma once
#include "os_api.h"

#if __unix__
#define OS_LINUX 1
#define OS_WINDOWS 0
#define OS_WASM 0
#include "os_linux.h"
#endif
