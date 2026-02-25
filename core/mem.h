// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// mem.h - Memory allocator
#pragma once
#include "chunk.h"
#include "type.h"

// =====================================
// Implementation
// =====================================
typedef struct Memory_Chunk Memory_Chunk;
struct Memory_Chunk {
    size_t size;
    Memory_Chunk *next;
};

// A stack allocator for variable size allocations.
// Each allocation should be smaller than the chunk size.
typedef struct {
    // List of allocated chunks
    Memory_Chunk *chunk;

    // Current chunk usage
    void *mem_start;
    void *mem_end;
} Memory;

// Align an integer to a power of two
static size_t size_align_up(size_t value, size_t align) {
    size_t mask = align - 1;
    return (value + mask) & ~mask;
}

// Align a pointer to a power of two
static void *ptr_align_up(void *ptr, size_t align) {
    size_t mask = align - 1;
    return (void *)(((intptr_t)ptr + mask) & ~mask);
}

// Allocate 'size' bytes of uninitialized memory
static void *mem_alloc_uninit(Memory *mem, size_t size) {
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
        // We need to allocate a new chunk.
        size_t header_size = size_align_up(sizeof(Memory_Chunk), align);
        Buffer buf = chunk_alloc(header_size + size);

        Memory_Chunk *chunk = (Memory_Chunk *)buf.data;
        chunk->size = buf.size;
        chunk->next = mem->chunk;
        mem->chunk = chunk;

        // Set new memory region
        mem->mem_start = buf.data + header_size;
        mem->mem_end = buf.data + buf.size;
        assert((intptr_t)mem->mem_start % align == 0);
        assert(mem->mem_end - mem->mem_start >= size);
    }

    // Allocate the memory from the current chunk
    void *alloc_start = mem->mem_start;
    mem->mem_start += size;
    assert(mem->mem_start <= mem->mem_end);
    return alloc_start;
}

// Allocate exactly 'size' bytes of zero initialized memory
static void *mem_alloc_zero(Memory *mem, size_t size) {
    void *ptr = mem_alloc_uninit(mem, size);
    mem_zero(ptr, size);
    return ptr;
}

static void *mem_clone(Memory *mem, void *data, size_t size) {
    void *new_data = mem_alloc_uninit(mem, size);
    mem_copy(new_data, data, size);
    return new_data;
}

// Allocate a zero-initialised type
#define mem_struct(MEM, TYPE) ((TYPE *)mem_alloc_zero((MEM), sizeof(TYPE)))

// Allocate an un-initialised array
#define mem_array(MEM, TYPE, N) ((TYPE *)mem_alloc_uninit((MEM), sizeof(TYPE) * (N)))

// Allocate an initialised array
#define mem_array_zero(MEM, TYPE, N) ((TYPE *)mem_alloc_zero((MEM), sizeof(TYPE) * (N)))

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
