#include "os.h"
#include "fmt.h"

void os_main(u32 argc, const char **argv) {
    test_fmt();
    os_exit(0);
}
