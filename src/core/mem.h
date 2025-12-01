// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#pragma once
#include "os.h"

typedef struct {
} Memory;

static Memory *mem_new(u32 size);
static void *mem_alloc_uninit(Memory *mem, u32 size);
static void *mem_alloc(Memory *mem, u32 size);

#define mem_struct(MEM, TYPE) (TYPE) mem_alloc_zero(MEM, sizeof(TYPE))
#define mem_array(MEM, TYPE, N) (TYPE) mem_alloc_uninit(MEM, sizeof(TYPE) * N)
