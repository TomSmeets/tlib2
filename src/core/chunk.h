// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
#pragma once
#include "os.h"
#include "type.h"

// General purpose allocator
// - use only for big allocations in MB range
// - Optimized for allocations of exactly 1 MB
// - Chunk size is 64K
// - Min allocation is 1 Mb
// - Max allocation is 256 Gb
// - For many small objects use 'Memory' arenas
static void *gpa_alloc(u64 size);
static void gpa_free(void *ptr);
static u64 gpa_size(void *ptr);

// =============== INTERNALS =============
#define CHUNK_SIZE (64 * 1024)

typedef struct Allocation Allocation;
struct Allocation {
    bool used;
    u32 count;
    Allocation *next;
};

static Allocation *gpa_alloc_raw(u32 count);
static void gpa_free_raw(Allocation *alloc);

static void *gpa_alloc(u64 size) {
}
static void gpa_free(void *ptr) {
}

static u64 gpa_size(void *ptr) {
    (Allocation
}

// Memory is made of a list of allocated fixed size chunks that cannot be freed
// Once a chunk is allocated from the os it is forever manged by us
typedef struct Allocation Allocation;
struct Allocation {
    bool used;
    u32 count;
    Allocation *next;
};

// Maximally compacted list of allocations
static Allocation *gpa_memory;

// Allocate a sequence of at least 'count' chunks
static Allocation *gpa_alloc_raw(u32 count) {
    for (Allocation *a = gpa_memory; a; a = a->next) {
        if (a->used) continue;

        // Too small
        if (a->count < real_count) continue;

        // Exact match
        if (a->count == real_count) {
            a->used = true;
            return (void *)(a + 1);
        }

        // Too big
        if (a->count > real_count) {
            Allocation *c = a->next;

            Allocation *b = (void *)a + real_count * CHUNK_SIZE;
            b->used = false;
            b->count = a->count - real_count;
            a->count = real_count;

            // Add b between a and a->next
            b->next = a->next;
            a->next = b;
        }

        if (a->size < real_size) continue;
        if (a->size > real_size) {
            Allocation *b = (void *)a + real_size;
            b->size = a->size - size;

            a->size = size;
            b->next = a->next;
            a->next = b;
        };
    }
}

static void gpa_alloc(u64 size) {
    u32 count = (size + sizeof(Allocation) + CHUNK_SIZE - 1) / CHUNK_SIZE;
    Allocation *all
}
// Free a chunk to the cache
static void gpa_free(Chunk *chunk);

// Resize a chunk if possible
static bool chunk_resize(Chunk *chunk, u32 count);

// ===============================================================

// List of maximally compacted chunks
static Chunk *chunk_cache;

static void chunk_free(Chunk *add) {
    void *add_start = add;
    void *add_end = add + add->size;

    Chunk **ref = &chunk_cache;
    for (Chunk *other = chunk_cache; other; other = other->next) {
        void *other_start = other;
        void *other_end = other + other->size;

        if (add_start == other_end) {
            other->size += add->size;
        }

        if (add_end == other_start) {
            add->size += other->size;
        }
    }
}

static Chunk *chunk_alloc(u32 count) {
    u32 match_count = 0;
    void *start = chunk_cache;
    void *end = chunk_cache;
    for (Chunk *c = chunk_cache; c; c = c->next) {
        if ((end - start) >= count)
            if (c == end) end += CHUNK_SIZE;

        Chunk *expect = start + CHUNK_SIZE if (c == match_item)
    }

    Chunk *chunk = os_alloc(count * CHUNK_SIZE);
    chunk->count = count;
    return chunk;
}

// Try to grow the chunk size
static void chunk_resize(Chunk *chunk, u32 new_count) {
}

static Chunk chunk_split(Chunk *chunk, u32 count) {
    if(chunk->
}

static Chunk chunk_free(Chunk *chunk) {
}
