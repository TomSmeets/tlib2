// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// mem.h - Memory allocator
#pragma once
#include "core/chunk.h"

typedef struct Memory Memory;

// Create a new memory allocator
static Memory *mem_new(void);

// Free this memory allocator and all it's allocations
static void mem_free(Memory *mem);

// Basic memory allocation methods
static void *mem_alloc_uninit(Memory *mem, u32 size);
static void *mem_alloc_zero(Memory *mem, u32 size);

// Allocate a zero-initialised type
#define mem_struct(MEM, TYPE) ((TYPE *)mem_alloc_zero((MEM), sizeof(TYPE)))

// Allocate an un-initialised array
#define mem_array(MEM, TYPE, N) ((TYPE *)mem_alloc_uninit((MEM), sizeof(TYPE) * (N)))

// A stack allocator for variable size allocations.
// Each allocation should be smaller than the chunk size.
struct Memory {
    // A list of used chunks.
    // The first entry is still being used for new allocations.
    Chunk *chunk;

    // Bytes used in the current chunck
    // This includes the chunk header
    u32 used;

    // Total size of the first chunk
    u32 size;
};

// Create a new memory allocator
static Memory *mem_new(void) {
    Memory mem = {};
    Memory *mem_ptr = mem_struct(&mem, Memory);
    *mem_ptr = mem;
    return mem_ptr;
}

// Free this memory allocator and all it's allocations
static void mem_free(Memory *mem) {
    Chunk *chunk = mem->chunk;
    mem->chunk = 0;
    mem->size = 0;
    mem->used = 0;
    while (chunk) {
        Chunk *next = chunk->next;
        chunk_free(chunk);
        chunk = next;
    }
}

// Align the next allocation to 16 bytes
static u32 u32_align_up(u32 value, u32 align) {
    u32 mask = align - 1;
    return (value + mask) & ~mask;
}

// Allocate 'size' bytes of uninitialized memory
static void *mem_alloc_uninit(Memory *mem, u32 size) {
    // Primitives should be aligned to their own size.
    //   int8 -> no alignment needed
    //   int32 -> 4 byte alignment
    //   int64 -> 8 byte alignment
    //   float -> 4 byte alignment
    //
    // SIMD registers should also be aligned
    //   float4 -> 16 byte alignment
    //
    // Assuming 16 byte alignment for all allocations
    u32 align = 16;
    mem->used = u32_align_up(mem->used, align);

    // Check if the allocation will fit
    if (mem->used + size > mem->size) {
        // The allocation odes not fit in the current chunk.
        // We need to allocat a new chunk.
        Chunk *chunk = chunk_alloc(size + sizeof(Chunk));

        // This chunk is the new 'current'
        // Fhe previous chunk is now full
        chunk->next = mem->chunk;
        mem->chunk = chunk;

        // Chunks are always the same size
        mem->size = chunk->size;
        mem->used = sizeof(Chunk);

        // Redo the alignment
        mem->used = u32_align_up(mem->used, align);

        // This allocation does not fit
        assert(mem->used + size <= mem->size);
    }

    // Allocate the memory from the current
    void *ptr = (void *)mem->chunk + mem->used;
    mem->used += size;
    return ptr;
}

// Allocate exactly 'size' bytes of zero initialized memory
static void *mem_alloc_zero(Memory *mem, u32 size) {
    void *ptr = mem_alloc_uninit(mem, size);
    std_memzero(ptr, size);
    return ptr;
}
