// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// write.h - Dynamic Buffer writer
#pragma once
#include "buf.h"
#include "mem.h"

typedef struct {
    Buffer buffer;
    size_t bytes_written;
    size_t bits_written;

    // Optional, allow reallocation when set
    Memory *mem;
} Write;

// Create a new writer that writes to the destination buffer
static Write write_from(Buffer buffer) {
    return (Write) { .buffer = buffer };
}

// Create a new dynamic writer that dynamically allocates memory
static Write *write_new(Memory *mem) {
    Write *write = mem_struct(mem, Write);
    write->mem = mem;
    return write;
}
