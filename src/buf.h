#pragma once
#include "type.h"
#include "str.h"

static Buffer buf_str(const char *str) {
    return (Buffer){
        (void *)str,
        str_len(str),
    };
}

static bool buf_starts_with(Buffer buf, Buffer start) {
    if (buf.size < start.size) return false;
    return std_memcmp(buf.data, start.data, start.size);
}
