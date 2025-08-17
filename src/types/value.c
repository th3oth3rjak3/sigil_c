// File:    value.c
// Purpose: implementation of value.h
// Author:  Jake Hathaway
// Date:    2025-08-17

#include "include/value.h"
#include "include/memory.h"
#include <stdio.h>

void
init_value_array(ValueArray* array) {
    array->values = NULL;
    array->count = 0;
    array->capacity = 0;
}

void
write_value_array(Allocator allocator, ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(
            allocator, Value, array->values, old_capacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void
free_value_array(Allocator allocator, ValueArray* array) {
    FREE_ARRAY(allocator, Value, array->values, array->capacity);
    init_value_array(array);
}

void
print_value(Value value) {
    printf("%g", value);
}
