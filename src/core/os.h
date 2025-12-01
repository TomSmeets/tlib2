// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#pragma once
#include "core/os_api.h"

#if __unix__
#define OS_LINUX 1
#define OS_WINDOWS 0
#define OS_WASM 0
#include "core/os_linux.h"
#endif
