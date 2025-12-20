// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// chunk.h - Memory allocator for big chunks
#pragma once
#include "buf.h"
#include "os.h"
#include "type.h"

typedef struct Chunk_Freelist Chunk_Freelist;
struct Chunk_Freelist {
    // Next chunk
    Chunk_Freelist *next;
};

// Min ->  1 MB
// Max ->  1 TB
#define CHUNK_CLASS_MIN (20)
#define CHUNK_CLASS_MAX (40)
#define CHUNK_SIZE_MIN ((size_t)1 << CHUNK_CLASS_MIN)
#define CHUNK_SIZE_MAX ((size_t)1 << CHUNK_CLASS_MAX)
static Chunk_Freelist *chunk_cache[CHUNK_CLASS_MAX - CHUNK_CLASS_MIN];

typedef struct {
    size_t size;
    Chunk_Freelist **cache;
} Chunk_Class;

// Calculate smallest N for which size <= 2^N
static u32 size_bits(size_t size) {
    if (size == 0) return 1;
    if (sizeof(size) == sizeof(u32)) return 32 - __builtin_clz(size - 1);
    if (sizeof(size) == sizeof(u64)) return 64 - __builtin_clzll(size - 1);
    os_fail("Invalid size");
}

static Chunk_Class chunk_class(size_t size) {
    u32 bits = size_bits(size);
    if (bits < CHUNK_CLASS_MIN) bits = CHUNK_CLASS_MIN;
    if (bits >= CHUNK_CLASS_MAX) os_fail("Invlaid size");

    size_t chunk_size = (size_t)1 << bits;
    Chunk_Freelist **list = &chunk_cache[ bits - CHUNK_CLASS_MIN];
    return (Chunk_Class){
        .size = (size_t)1 << bits,
        .cache = &chunk_cache[ bits - CHUNK_CLASS_MIN],
    };
}

static Buffer chunk_alloc(size_t size) {
    Chunk_Class class = chunk_class(size);
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

static void chunk_free(void *data, size_t size) {
    Chunk_Class class = chunk_class(size);

    // Add chunk to cache
    Chunk_Freelist *chunk = data;
    chunk->next = *class.cache;
    *class.cache = chunk;
}
