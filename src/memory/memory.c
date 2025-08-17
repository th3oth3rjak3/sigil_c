// File:    memory.c
// Purpose: Implement memory.h
// Author:  Jake Hathaway
// Date:    2025-08-17

#include "include/memory.h"
#include <stdio.h>
#include <stdlib.h>

/* ======================= System Allocator Interface ======================= */

static void*
system_alloc(size_t size) {
    return malloc(size);
}

static void
system_free(void* ptr, size_t old_size) {
    free(ptr);
}

static void*
system_realloc(void* ptr, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(ptr);
        return NULL;
    }

    void* result = realloc(ptr, newSize);
    if (result == NULL)
        exit(1);
    return result;
}

static void
system_report_statistics(void) {}

/* ======================== Global System Allocator ========================= */

// Get the default libc system allocator.
Allocator
get_system_allocator(void) {
    Allocator alloc = {
        .alloc = system_alloc,
        .free = system_free,
        .realloc = system_realloc,
        .report_statistics = system_report_statistics,
    };

    return alloc;
}

/* ======================= Debug Allocator Interface ======================== */

static size_t total_allocated = 0;
static size_t alloc_count = 0;
static size_t free_count = 0;
static size_t peak_allocated = 0;

static void*
debug_alloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        total_allocated += size;
        if (total_allocated > peak_allocated) {
            peak_allocated = total_allocated;
        }

        alloc_count++;
        printf(
            "[ALLOC] %zu bytes at %p (total: %zu)\n",
            size,
            ptr,
            total_allocated);
    }
    return ptr;
}

static void
debug_free(void* ptr, size_t old_size) {
    if (ptr) {
        free_count++;
        total_allocated -= old_size;
        printf("[FREE]  %p (frees: %zu)\n", ptr, free_count);
        free(ptr);
    }
}

static void*
debug_realloc(void* ptr, size_t old_size, size_t new_size) {
    if (new_size == 0) {
        debug_free(ptr, old_size);
        printf("[REALLOC] Freed %p (old: %zu, new: 0)\n", ptr, old_size);
        return NULL;
    }

    printf("[REALLOC] %p (old: %zu) -> ", ptr, old_size);
    void* new_ptr = realloc(ptr, new_size);

    if (!new_ptr) {
        printf("FAILED (requested %zu bytes)\n", new_size);
        exit(1);
    }

    // Update statistics
    total_allocated += new_size;
    if (total_allocated > peak_allocated) {
        peak_allocated = total_allocated;
    }
    alloc_count++;
    printf(
        "%p (new: %zu) | Total: %zu bytes\n",
        new_ptr,
        new_size,
        total_allocated);

    return new_ptr;
}

static void
debug_report_statistics(void) {
    printf("\n=== Memory Report ===\n");
    printf("Current:   %zu bytes\n", total_allocated);
    printf("Peak:      %zu bytes\n", peak_allocated);
    printf("Allocs:    %zu\n", alloc_count);
    printf("Frees:     %zu\n", free_count);

    if (alloc_count > free_count) {
        printf("\nLeaks (%zu):\n", alloc_count - free_count);
    }
    printf("====================\n");
}

/* ========================= Global Debug Allocator ========================= */

// Get a debug allocator for debugging.
Allocator
get_debug_allocator(void) {
    Allocator alloc = {
        .alloc = debug_alloc,
        .free = debug_free,
        .realloc = debug_realloc,
        .report_statistics = debug_report_statistics,
    };

    return alloc;
}
