// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// os_exit.h
#pragma once
#include "error.h"
#include "os_headers.h"
#include "str.h"

// Exit current application with optionally an error
__attribute__((__noreturn__)) static void os_exit(void) {
    if (error) {
        // Exit with an error
#if OS_LINUX
        linux_write(2, error, str_len(error));
        linux_exit_group(1);
#elif OS_WINDOWS
        MessageBoxA(NULL, error, "Error", MB_ICONERROR | MB_OK);
        ExitProcess(1);
#elif OS_WASM
        wasm_call_vp(
            "(msg) => {"
            "  console.log(str_c(msg));"
            "  alert(str_c(msg));"
            "  tlib.exit = true;"
            "}",
            error
        );
#endif
    } else {
        // Exit normally
#if OS_LINUX
        linux_exit_group(0);
#elif OS_WINDOWS
        ExitProcess(0);
#elif OS_WASM
        wasm_call("() => tlib.exit = true");
#endif
    }

    // Issue invalid instruction
    // Ensures exit if the os did not do it's job
    __builtin_trap();
}
