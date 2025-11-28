#pragma once
#include "os.h"

typedef struct {
} Memory;

static Memory *mem_new(u32 size);
static void *mem_alloc(Memory *mem, u32 size);

#define mem_struct(MEM, TYPE) (TYPE) mem_alloc(MEM, sizeof(TYPE))
