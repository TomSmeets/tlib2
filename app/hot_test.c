#include <stdio.h>
#include <unistd.h>

static int counter = 0;

void os_main(int argc, char **argv) {
    printf("Hello world: %d!\n", counter);
    counter++;
    usleep(100*1000); // sleep 5 ms
}

// Used when compiled as a normal application
int main(void) {
    while (1) os_main(0, 0);
}
