// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// parse.h - Recusive decent text parser
#pragma once
#include "type.h"

typedef struct Parse Parse;

struct Parse {
    u64 len;
    u8 *data;
};

static void parse_new(Memory *mem, u64 len, void *data) {
};
