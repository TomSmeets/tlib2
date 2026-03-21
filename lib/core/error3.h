#pragma once
#include "type.h"
#include "buf.h"

typedef struct {
    char *line;
    char *func;
    char *code;
} Error_Line;

typedef struct {
    u32 count;
    Error_Line stack[16];
} Error;

static _Thread_local Error error;

#define try(X) if ((error.count || !(X)) && error_add(__FILE__ ":" TO_STRING(__LINE__), (char *)__FUNCTION__, "try(" #X ")"))
#define check() if (error.count && error_add(__FILE__ ":" TO_STRING(__LINE__), __FUNCTION__, "check()"))
#define ok() error_clear()

// Add another error to the list of messages
static bool error_add(char *line, char *func, char *code) {
    if (error.count > array_count(error.stack)) return 1;
    error.stack[error.count++] = (Error_Line){line, func, code};
    return 1;
}

static void error_clear(void) {
    error.count = 0;
}

static int div(int a, int b) {
    try(b > 0) return 0;
    return a / b;
}

static int os_system(char *cmd) {
    int ret = system(cmd);
    try(ret == 0);
    return ret;
}

static Buffer os_read(char *f) {
    try(f) return buf_null();
    return buf_null();
}

static i32 os_system(char *command) {
    errror = __FUNCTION__
}

// int div2(int a, int b, int c) {
//     int x = div(a, b);
//     int y = div(a, c);
//     if(error) return 0;
//     return x + y;
// }
