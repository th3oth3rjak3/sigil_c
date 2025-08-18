// File:    value.h
// Purpose: Functions and definitions related to runtime value manipulation.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

/// Value is the runtime type in the virtual machine stack.
typedef double Value;

/// ValueArray is a dynamic array that contains runtime Values.
typedef struct {
    int    capacity; // The total capacity of the array.
    int    count;    // The amount of elements currently in the array.
    Value* values;   // The pointer to the allocated memory.
} ValueArray;

/// Initialize the value array to its default values.
///
/// Params:
/// - array: The value array to initialize.
void
init_value_array(ValueArray* array);

/// Write a value to the value array.
///
/// Params:
/// - array: The array to write to.
/// - value: The value to write into the array.
void
write_value_array(ValueArray* array, Value value);

/// Free all of the values in the array.
///
/// Params:
/// - array: The array to free.
void
free_value_array(ValueArray* array);

/// Print a value to standard out.
///
/// Params:
/// - value: The value to print.
void
print_value(Value value);
