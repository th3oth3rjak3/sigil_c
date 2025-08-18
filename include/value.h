// File:    value.h
// Purpose: Functions and definitions related to runtime value manipulation.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

#include <stdbool.h>

typedef struct Obj       Obj;
typedef struct ObjString ObjString;

/// ValueType is a type tag to differentiage between the different runtime
/// value types.
typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

/// Value is the runtime type in the virtual machine stack.
typedef struct {
    ValueType type;
    union {
        bool   boolean;
        double number;
        Obj*   obj;
    } as;
} Value;

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object) ((Value){VAL_OBJ, {.obj = (Obj*)object}})

/// ValueArray is a dynamic array that contains runtime Values.
typedef struct {
    int    capacity; // The total capacity of the array.
    int    count;    // The amount of elements currently in the array.
    Value* values;   // The pointer to the allocated memory.
} ValueArray;

/// Compare two values for equality.
bool
values_equal(Value a, Value b);

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
