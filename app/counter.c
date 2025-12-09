#include "os.h"
#include "fmt.h"
#include "type.h"

static u32 counter1;
static u32 counter2;
static u32 counter3;
static const char *message = "Hello World!";

void os_main(u32 argc, char **argv) {
    fmt_su(fout, "Counter1: ", counter1, "\n");
    fmt_su(fout, "Counter2: ", counter2, "\n");
    fmt_su(fout, "Counter3: ", counter3, "\n");
    fmt_ss(fout, "msg: ", (char*)message, "\n");
    // message[0] = 'X';

    counter1++;
    if (counter1 > 10) {
        counter2++;
        counter1 = 0;
    }
    if (counter2 > 10) {
        counter3++;
        counter2 = 0;
    }
    os_sleep(1ULL * 1000 * 1000);
}
