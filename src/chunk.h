// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// Memory allocator for big chunks that can be reused
#pragma once
#include "core/os.h"
#include "core/type.h"

typedef struct {
    void *data;
    u64 size;
} Buffer;

// Allocate a chunk of a given minimum size
static Buffer chunk_alloc(u64 size);

// Free the chunk
static void chunk_free(void *data, u64 size);

// ==================================================
//                    Internals
// ==================================================
typedef struct Chunk_Freelist Chunk_Freelist;
struct Chunk_Freelist {
    // Next chunk
    Chunk_Freelist *next;
};

// Min ->  1 MB
// Max -> 64 GB
#define CHUNK_SIZE (1ULL * 1024 * 1024)
static Chunk_Freelist *chunk_cache[16];

typedef struct {
    u64 size;
    Chunk_Freelist **cache;
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

static Buffer chunk_alloc(u64 size) {
    Chunk_Class class = _chunk_class(size);
    Chunk_Freelist *cached_chunk = *class.cache;

    // Allocate new memory when no cached chunk is found
    if (!cached_chunk) {
        void *data = os_alloc(class.size);
        return (Buffer){data, class.size};
    }

    // Remove from freelist
    *class.cache = cached_chunk->next;
    return (Buffer){(void *)cached_chunk, class.size};
}

static void chunk_free(void *data, u64 size) {
    Chunk_Class class = _chunk_class(size);

    // Add chunk to cache
    Chunk_Freelist *chunk = data;
    chunk->next = *class.cache;
    *class.cache = chunk;
}
