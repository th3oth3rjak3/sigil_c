// File:    object.h
// Purpose: Definitions related to the Object type.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

#include "src/types/value.h"

// Get the type of the object.
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

// Determine if the object is a string.
#define IS_STRING(value) is_obj_type(value, OBJ_STRING)

// Convert the object to an ObjString type.
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))

// Convert the object to a raw C string.
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

/// A tag to identify the different types of objects supported
/// in the language.
typedef enum {
    OBJ_STRING, // A string.
} ObjType;

/// An object instance.
struct Obj {
    ObjType type; // The type of the object.
    Obj*    next; // An intrusive list containing a pointer to the next object.
};

/// A string representation.
struct ObjString {
    Obj      obj;    // The object header.
    int      length; // The number of characters
    char*    chars;  // The pointer to the string contents.
    uint32_t hash;   // The precomputed hash for the string.
};

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
