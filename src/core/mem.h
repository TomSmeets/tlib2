// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// mem.h - Memory allocator
#pragma once
#include "core/chunk.h"
#include "core/type.h"

typedef struct Memory Memory;

// Create a new memory allocator
static Memory *mem_new(void);

// Free this memory allocator and all it's allocations
static void mem_free(Memory *mem);

// Basic memory allocation methods
static void *mem_alloc_uninit(Memory *mem, u64 size);
static void *mem_alloc_zero(Memory *mem, u64 size);

// Allocate a zero-initialised type
#define mem_struct(MEM, TYPE) ((TYPE *)mem_alloc_zero((MEM), sizeof(TYPE)))

// Allocate an un-initialised array
#define mem_array(MEM, TYPE, N) ((TYPE *)mem_alloc_uninit((MEM), sizeof(TYPE) * (N)))

// =====================================
// Implementation
// =====================================
typedef struct Memory_Chunk Memory_Chunk;
struct Memory_Chunk {
    u64 size;
    Memory_Chunk *next;
};

// A stack allocator for variable size allocations.
// Each allocation should be smaller than the chunk size.
struct Memory {
    // List of allocated chunks
    Memory_Chunk *chunk;

    // Current chunk usage
    void *mem_start;
    void *mem_end;
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
    Memory_Chunk *chunk = mem->chunk;
    while (chunk) {
        Memory_Chunk *next = chunk->next;
        chunk_free(chunk, chunk->size);
        chunk = next;
    }
}

// Align a u64 integer to a power of two
static u64 u64_align_up(u64 value, u64 align) {
    u64 mask = align - 1;
    return (value + mask) & ~mask;
}

// Align a pointer to a power of two
static void *ptr_align_up(void *ptr, u64 align) {
    return (void *)u64_align_up((u64)ptr, align);
}

// Allocate 'size' bytes of uninitialized memory
static void *mem_alloc_uninit(Memory *mem, u64 size) {
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
    mem->mem_start = ptr_align_up(mem->mem_start, align);

    // Check if the allocation will fit
    if (mem->mem_start + size > mem->mem_end) {
        // The allocation odes not fit in the current chunk.
        // We need to allocat a new chunk.
        u64 header_size = u64_align_up(sizeof(Memory_Chunk), align);
        Buffer buf = chunk_alloc(header_size + size);

        Memory_Chunk *chunk = buf.data;
        chunk->size = buf.size;
        chunk->next = mem->chunk;
        mem->chunk = chunk;

        // Set new memory region
        mem->mem_start = buf.data + header_size;
        mem->mem_end = buf.data + buf.size;
        assert((u64)mem->mem_start % align == 0);
        assert(mem->mem_end - mem->mem_start >= size);
    }

    // Allocate the memory from the current
    void *alloc_start = mem->mem_start;
    mem->mem_start += size;
    assert(mem->mem_start <= mem->mem_end);
    return alloc_start;
}

// Allocate exactly 'size' bytes of zero initialized memory
static void *mem_alloc_zero(Memory *mem, u64 size) {
    void *ptr = mem_alloc_uninit(mem, size);
    std_memzero(ptr, size);
    return ptr;
}
