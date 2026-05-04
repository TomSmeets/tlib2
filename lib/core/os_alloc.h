// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// alloc.h - Platform independent memory allocation
#pragma once
#include "error.h"
#include "os_headers.h"
#include "type.h"

// Allocate a new chunk of memory
static void *os_alloc(size_t size) {
#if OS_LINUX
    // see 'man 2 mmap'
    void *ptr = linux_mmap(
        // Let the system choose a starting address for us
        0,
        // Allocation size
        size,
        // Read and writable, not executable
        PROT_READ | PROT_WRITE,
        // Memory is for this process only
        // Memory is not backed by a file, fd must be -1, and offset must be 0
        MAP_PRIVATE | MAP_ANONYMOUS,
        // fd, offset
        -1, 0
    );

    // Check if mapping was ok
    check(ptr != MAP_FAILED);
#elif OS_WINDOWS
    void *ptr = VirtualAlloc(
        // Let the system choose a starting address for us
        0,
        // Allocation size
        size,
        // Reserve the address range and commit the memory in that range
        MEM_COMMIT | MEM_RESERVE,
        // Memory should be read and writable
        PAGE_READWRITE
    );
#elif OS_WASM
    // Get number of pages rounded up
    size_t pages = (size + WASM_PAGE_SIZE - 1) / WASM_PAGE_SIZE;

    // Grow memory and return first index of the new pages
    size_t page_ix = wasm_memory_grow(pages);

    // Check allocation result, -1 is failure
    check(page_ix != (size_t)-1);

    // Convert to pointer
    void *ptr = (void *)(page_ix * WASM_PAGE_SIZE);
#endif
    // We consider null to also be invalid
    check(ptr != 0);
    return ptr;
}

