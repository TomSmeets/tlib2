// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// hot_test.h - Test application for hot reloading
#include <stdio.h>
#include <unistd.h>

static int counter = 0;

void os_main(int argc, char **argv) {
    printf("Hello world: %d!\n", counter);
    counter++;
    usleep(100 * 1000); // sleep 5 ms
}

// Used when compiled as a normal application
int main(int argc, char **argv) {
    while (1) os_main(argc, argv);
}
