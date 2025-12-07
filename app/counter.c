#include "os.h"
#include "type.h"
#include <stdio.h>

static u32 counter1;
static u32 counter2;
static u32 counter3;

const char *message = "Hello World!";

void os_main(u32 argc, char **argv) {
    printf("Counter1: %u\n", counter1);
    printf("Counter2: %u\n", counter2);
    printf("Counter3: %u\n", counter3);
    printf("msg: %s\n", message);

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
