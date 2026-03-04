#pragma once
#include "error.h"
#include "os.h"

static void error_exit(void) {
    if (error_count == 0) os_exit(0);
    for (u32 i = 0; i < error_count; ++i) {
        os_write(os_stderr(), error_message[i], str_len(error_message[i]), 0);
    }
    os_exit(1);
}
