// File:    memory.c
// Purpose: Implement memory.h
// Author:  Jake Hathaway
// Date:    2025-08-17

#include "src/memory/memory.h"
#include "bytecode.h"
#include "src/runtime/vm.h"
#include "src/types/object.h"
#include "src/types/value.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PRINT_ALLOCATIONS
static size_t total_allocated = 0;
static size_t alloc_count = 0;
static size_t free_count = 0;
static size_t peak_allocated = 0;
#endif

void*
reallocate(void* pointer, size_t old_size, size_t new_size) {
    void* result;

    if (new_size == 0) {
#ifdef DEBUG_PRINT_ALLOCATIONS
        if (pointer) {
            total_allocated -= old_size;
            free_count++;
            printf(
                "[FREE]  %p (old: %zu) | Total: %zu bytes\n",
                pointer,
                old_size,
                total_allocated);
        }
#endif
        free(pointer);
        return NULL;
    }

    if (pointer == NULL) {
        // New allocation
        result = malloc(new_size);
#ifdef DEBUG_PRINT_ALLOCATIONS
        if (result) {
            total_allocated += new_size;
            if (total_allocated > peak_allocated) {
                peak_allocated = total_allocated;
            }
            alloc_count++;
            printf(
                "[ALLOC] %zu bytes at %p | Total: %zu bytes\n",
                new_size,
                result,
                total_allocated);
        }
#endif
    } else {
        // Reallocation
#ifdef DEBUG_PRINT_ALLOCATIONS
        printf("[REALLOC] %p (old: %zu) -> ", pointer, old_size);
#endif
        result = realloc(pointer, new_size);
#ifdef DEBUG_PRINT_ALLOCATIONS
        if (result) {
            total_allocated = total_allocated - old_size + new_size;
            if (total_allocated > peak_allocated) {
                peak_allocated = total_allocated;
            }
            alloc_count++;
            free_count++;
            printf(
                "%p (new: %zu) | Total: %zu bytes\n",
                result,
                new_size,
                total_allocated);
        }
#endif
    }

    if (result == NULL) {
        exit(1);
    }

    return result;
}

#ifdef DEBUG_PRINT_ALLOCATIONS
void
report_memory_statistics(void) {
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
#endif

static void
free_object(Obj* object) {
    switch (object->type) {
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            free_bytecode(&function->bytecode);
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_NATIVE: {
            FREE(ObjNative, object);
            break;
        }
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
    }
}

void
free_objects(void) {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        free_object(object);
        object = next;
    }
}
