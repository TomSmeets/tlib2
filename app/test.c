#include "core/os.h"
#include "std/fmt.h"

void os_main(u32 argc, const char **argv) {
    test_fmt();
    os_exit(0);
}
