// File:    memory.h
// Purpose: Define Allocator interface and memory operations.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

#include <stddef.h>
#include <stdint.h>

/// Allocator is an interface for memory operations to enable dependency
/// injection and support testing scenarios.
typedef struct {
    /// Allocate memory
    ///
    /// Params:
    /// - size: The size in bytes to allocate.
    ///
    /// Returns:
    /// - void*: A pointer to the allocated memory.
    void* (*alloc)(size_t size);

    /// Free the memory at the pointer location.
    ///
    /// Params:
    /// - ptr: The pointer to the memory to free.
    /// - oldSize: The size of the existing allocation.
    void (*free)(void* ptr, size_t oldSize);

    /// Reallocate from the current pointer to a new pointer
    /// for the given size.
    ///
    /// Params:
    /// - ptr: The existing pointer.
    /// - old_size: The existing size.
    /// - new_size: The size of the new_allocation.
    ///
    /// Returns:
    /// - void*: The newly allocated pointer.
    void* (*realloc)(void* ptr, size_t oldSize, size_t newSize);

    /// Print statistics about the allocator.
    void (*report_statistics)(void);
} Allocator;

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define ALLOC_WITH(allocator, type, count)                                     \
    (type*)(allocator).alloc(sizeof(type) * (count))

#define REALLOC_WITH(allocator, type, ptr, old_count, new_count)               \
    (type*)(allocator).realloc(                                                \
        ptr, sizeof(type) * (old_count), sizeof(type) * (new_count))

#define FREE_WITH(allocator, type, ptr, old_count)                             \
    (allocator).free(ptr, sizeof(type) * old_count)

// Convenience wrapper for GROW_ARRAY pattern
#define GROW_ARRAY(allocator, type, pointer, old_count, new_count)             \
    (REALLOC_WITH(allocator, type, pointer, old_count, new_count))

#define FREE_ARRAY(allocator, type, pointer, old_count)                        \
    (FREE_WITH(allocator, type, pointer, old_count))

extern const Allocator GlobalAllocator;

void
free_objects();
