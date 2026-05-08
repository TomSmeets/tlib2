// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os.h - Operating System API
#pragma once
#include "os_alloc.h"
#include "os_api.h"
#include "os_exit.h"

#if OS_LINUX
#include "os_linux.h"
#elif OS_WINDOWS
#include "os_windows.h"
#endif
