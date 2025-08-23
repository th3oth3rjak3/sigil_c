// File:    memory.h
// Purpose: Define Allocator interface and memory operations.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

#include "object.h"
#include "value.h"
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

/// Reallocate memory. When old_size is 0, and new_size is non-zero,
/// allocate. When new_size is 0 and old_size is non-zero, free.
///
/// Params:
/// - pointer: The pointer to the memory location to allocate.
/// - old_size: The size in bytes of the existing memory.
/// - new_size: The size in bytes of the desired memory.
///
/// Returns:
/// - void*: A pointer to the newly allocated memory.
void*
reallocate(void* pointer, size_t old_size, size_t new_size);

/// Run the garbage collector to reclaim unused memory.
void
collect_garbage();

/// Mark a value as reachable so it's not collected.
///
/// Params:
/// - value: The value to mark as reachable.
void
mark_value(Value value);

/// Mark an object as reachable so it's not collected.
///
/// Params:
/// -object: The object header to set the mark bit on.
void
mark_object(Obj* object);

/// Free all objects that were allocated.
void
free_objects(void);

/// Report memory statistics and usage.
void
report_memory_statistics(void);
