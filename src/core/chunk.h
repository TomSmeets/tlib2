// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#pragma once
#include "core/os.h"
#include "core/type.h"

typedef struct Chunk Chunk;

struct Chunk {
    // Total chunk size, including header
    u64 size;

    // Next chunk
    Chunk *next;
};

// Allocate a chunk of a given minimum size
static Chunk *chunk_alloc(u64 size);

// Free the chunk
static void chunk_free(Chunk *chunk);

// =============== INTERNALS =============
// Min ->  1 MB
// Max -> 64 GB
#define CHUNK_SIZE (1ULL * 1024 * 1024)
static Chunk *chunk_cache[16];

typedef struct {
    u64 size;
    Chunk **cache;
    u32 class;
} Chunk_Class;

static Chunk_Class _chunk_class(u64 size) {
    u64 chunk_size = CHUNK_SIZE;
    u32 chunk_class = 0;
    while (chunk_size < size) {
        chunk_size *= 2;
        chunk_class++;
    }
    assert(chunk_class < array_count(chunk_cache));
    return (Chunk_Class){
        .size = chunk_size,
        .class = chunk_class,
        .cache = &chunk_cache[chunk_class],
    };
}

static Chunk *chunk_alloc(u64 size) {
    Chunk_Class class = _chunk_class(size);
    if (*class.cache) {
        Chunk *chunk = *class.cache;
        *class.cache = chunk->next;
        chunk->next = 0;
        return chunk;
    }

    Chunk *chunk = os_alloc(class.size);
    chunk->size = class.size;
    chunk->next = 0;
    return chunk;
}

static void chunk_free(Chunk *chunk) {
    Chunk_Class class = _chunk_class(chunk->size);
    chunk->next = *class.cache;
    *class.cache = chunk;
}
