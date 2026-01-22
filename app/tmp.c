#include "fmt.h"
#include "os.h"

void os_main(u32 argc, char **argv) {
    fmt_s(fout, "Hello World!\n");
    os_exit(0);
}
