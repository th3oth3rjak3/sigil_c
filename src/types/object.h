// File:    object.h
// Purpose: Definitions related to the Object type.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

#include "src/runtime/bytecode.h"
#include "src/types/value.h"
#include <stdint.h>

// Get the type of the object.
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

// Determine if the object is a string.
#define IS_STRING(value) is_obj_type(value, OBJ_STRING)

// Determine if the object is a function
#define IS_FUNCTION(value) is_obj_type(value, OBJ_FUNCTION)

// Determine if the object is a native function.
#define IS_NATIVE(value) is_obj_type(value, OBJ_NATIVE)

// Convert the object to an ObjString type.
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))

// Convert the object to an ObjFunction type.
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))

#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)

// Convert the object to a raw C string.
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

/// A tag to identify the different types of objects supported
/// in the language.
typedef enum {
    OBJ_FUNCTION, // A function
    OBJ_NATIVE,   // A native C function.
    OBJ_STRING,   // A string.
} ObjType;

/// An object instance.
struct Obj {
    ObjType type; // The type of the object.
    Obj*    next; // An intrusive list containing a pointer to the next object.
};

/// A function that can be called.
typedef struct {
    Obj        obj;      // The object header.
    int        arity;    // The number of function parameters.
    Bytecode   bytecode; // The bytecode for the function body.
    ObjString* name;     // The function name.
} ObjFunction;

/// A native C function.
///
/// Params:
/// - arg_count: The number of arguments it takes (arity)
/// - args: The actual arguments.
///
/// Returns:
/// - Value: A value result.
typedef Value (*NativeFn)(int argCount, Value* args);

/// A native C function.
typedef struct {
    Obj      obj;      // Object header.
    NativeFn function; // The C function pointer.
} ObjNative;

/// A string representation.
struct ObjString {
    Obj      obj;    // The object header.
    int      length; // The number of characters
    char*    chars;  // The pointer to the string contents.
    uint32_t hash;   // The precomputed hash for the string.
};

/// Create a new function.
///
/// Returns:
/// - ObjFunction*: A newly initialized function pointer.
ObjFunction*
new_function();

/// Create a new native function.
///
/// Params:
/// - function: The native C function pointer.
///
/// Returns:
/// - ObjNative*: A pointer to a new native function object.
ObjNative*
new_native(NativeFn function);

/// Create a deep copy of the string.
///
/// Params:
/// - chars: The pointer to the start of the string.
/// - length: The count of characters in the string.
///
/// Returns:
/// - ObjString*: A pointer to the newly allocated string object.
ObjString*
copy_string(const char* chars, int length);

/// Print the object to stdout.
///
/// Params:
/// - value: The value to print, assumed to be an object.
void
print_object(Value value);

/// Take ownership of the string.
///
/// Params:
/// - chars: The pointer to the start of the string.
/// - length: The count of characters in the string.
///
/// Returns:
/// - ObjString*: The owned string pointer.
ObjString*
take_string(char* chars, int length);

/// Check to see if the value is of the given object type.
///
/// Params:
/// - value: The value to check.
/// - type: The expected object type.
///
/// Returns:
/// - bool: True if the expected type matches, otherwise false.
static inline bool
is_obj_type(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
