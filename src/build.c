#include "core/os.h"

void os_main(u32 argc, const char **argv) {
    os_system("ls -l");
    os_exit(0);
}
