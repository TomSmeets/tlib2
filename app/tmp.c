#include "fmt.h"
#include "os.h"

#if OS_WASM
WASM_IMPORT(update_content) void update_content(u32 i);
#else
static void update_content(u32 i) {
}
#endif

static u32 i = 0;

void os_main(u32 argc, char **argv) {
    fmt_s(fout, "Hello World!\n");
    update_content(i++);
    os_sleep(TIME_SEC);
}
