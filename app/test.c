#include "fmt.h"
#include "os.h"

void os_main(u32 argc, char **argv) {
    test_fmt();
    os_exit(0);
}
