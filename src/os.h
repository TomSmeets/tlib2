// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os.h - Operating System API
#pragma once
#include "os_api.h"

#if OS_LINUX
#include "os_linux.h"
#elif OS_WINDOWS
#include "os_windows.h"
#elif OS_WASM
#include "os_wasm.h"
#endif
