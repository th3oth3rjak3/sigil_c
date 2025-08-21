// File:    memory.h
// Purpose: Define Allocator interface and memory operations.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

#include <stddef.h>
#include <stdint.h>

#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, old_count, new_count)                        \
    (type*)reallocate(                                                         \
        pointer, sizeof(type) * (old_count), sizeof(type) * (new_count))

#define FREE_ARRAY(type, pointer, old_count)                                   \
    reallocate(pointer, sizeof(type) * (old_count), 0)

void*
reallocate(void* pointer, size_t old_size, size_t new_size);

/// Free all objects that were allocated.
void
free_objects(void);

/// Report memory statistics and usage.
void
report_memory_statistics(void);
