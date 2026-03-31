// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// mem.h - Memory allocator
#pragma once
#include "chunk.h"
#include "ptr.h"
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
    if (!mem->mem_start || mem->mem_start + size > mem->mem_end) {
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
    if (ptr) ptr_zero(ptr, size);
    return ptr;
}

static void *mem_clone(Memory *mem, void *data, size_t size) {
    void *new_data = mem_alloc_uninit(mem, size);
    ptr_copy(new_data, data, size);
    return new_data;
}

// Allocate a zero-initialised type
#define mem_struct(MEM, TYPE) ((TYPE *)mem_alloc_zero((MEM), sizeof(TYPE)))

// Allocate an un-initialised array
#define mem_array(MEM, TYPE, N) ((TYPE *)mem_alloc_uninit((MEM), sizeof(TYPE) * (N)))

// Allocate an initialised array
#define mem_array_zero(MEM, TYPE, N) ((TYPE *)mem_alloc_zero((MEM), sizeof(TYPE) * (N)))

static Buffer mem_buffer(Memory *mem, size_t size) {
    void *ptr = mem_alloc_uninit(mem, size);
    return buf_from(ptr, size);
}

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

// Copy data into a newly allocated buffer with a given size
static u8 *mem_realloc(Memory *mem, u8 *old_data, size_t old_size, size_t new_size) {
    // Make sure at least the old data fits
    check_or(new_size >= old_size) new_size = old_size;
    u8 *new_data = mem_array(mem, u8, new_size);
    ptr_copy(new_data, old_data, old_size);
    return new_data;
}

static void test_mem(void) {
    size_t original_size1 = chunk_alloc_size;
    Memory *tmp = mem_new();
    size_t original_size = chunk_alloc_size;
    for (u32 i = 0; i < 8; ++i) {
        Memory *mem = mem_new();

        for (u32 j = 1; j < 32; ++j) {
            size_t size = j * j * j * j * 3;

            volatile u8 *data = mem_alloc_uninit(mem, size);
            for (size_t k = 0; k < size; ++k) {
                data[k] = k;
            }
        }

        check(chunk_alloc_size > original_size);
        mem_free(mem);
        check(chunk_alloc_size == original_size);
    }
    mem_free(tmp);
    check(chunk_alloc_size == original_size1);
}
