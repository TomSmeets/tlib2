#pragma once
#include "type.h"

typedef struct {
    char *file;
    char *line;
    char *function;
    char *expr;
} Error_Line;

static u32 error;
static Error_Line error_stack[8];

#define check(X) error_set0(__FILE__, TO_STRING(__LINE__), __FUNCTION__, "check(" #X ")", X)

static bool error_set0(const char *file, const char *line, const char *function, const char *expr, bool value) {
    if (value) return 0;
    if (error >= array_count(error_stack)) return 1;
    error_stack[error++] = (Error_Line){(char *)file, (char *)line, (char *)function, (char *)expr};
    return 1;
}
