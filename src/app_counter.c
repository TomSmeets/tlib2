#include "core/os.h"
#include "core/type.h"
#include <stdio.h>

static u32 counter1;
static u32 counter2;

void os_main(u32 argc, const char **argv) {
    printf("Counter1: %u\n", counter1);
    printf("Counter2: %u\n", counter2);
    counter1++;
    counter2++;
}
