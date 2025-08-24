// File:    value.h
// Purpose: Functions and definitions related to runtime value manipulation.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

#include "common.h"
#include <stdbool.h>
#include <string.h>

typedef struct Obj       Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN ((uint64_t)0x7ffc000000000000)

#define TAG_NIL 1   // 01.
#define TAG_FALSE 2 // 10.
#define TAG_TRUE 3  // 11.

typedef uint64_t Value;

#define NUMBER_VAL(num) num_to_value(num)

#define IS_NUMBER(value) (((value) & QNAN) != QNAN)

#define AS_NUMBER(value) value_to_num(value)

static inline Value
num_to_value(double num) {
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
}

static inline double
value_to_num(Value value) {
    double num;
    memcpy(&num, &value, sizeof(value));
    return num;
}

#define NIL_VAL ((Value)(uint64_t)(QNAN | TAG_NIL))
#define IS_NIL(value) ((value) == NIL_VAL)

#define FALSE_VAL ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define BOOL_VAL(b) ((b) ? TRUE_VAL : FALSE_VAL)
#define AS_BOOL(value) ((value) == TRUE_VAL)
#define IS_BOOL(value) (((value) | 1) == TRUE_VAL)

#define OBJ_VAL(obj) (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))
#define AS_OBJ(value) ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))
#define IS_OBJ(value) (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))
#else

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

// Check if value is a boolean.
#define IS_BOOL(value) ((value).type == VAL_BOOL)

// Check if value is nil.
#define IS_NIL(value) ((value).type == VAL_NIL)

// Check if value is a number.
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

// Check if value is an object.
#define IS_OBJ(value) ((value).type == VAL_OBJ)

// Read value as boolean.
#define AS_BOOL(value) ((value).as.boolean)

// Read value as number.
#define AS_NUMBER(value) ((value).as.number)

// Read value as an object.
#define AS_OBJ(value) ((value).as.obj)

// Create a boolean value
#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})

// Create a nil value
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})

// Create a number value
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

// Create an object value
#define OBJ_VAL(object) ((Value){VAL_OBJ, {.obj = (Obj*)object}})

#endif

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

ObjString*
number_to_string(double value);