// File:    object.h
// Purpose: Definitions related to the Object type.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) is_obj_type(value, OBJ_STRING)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_STRING,
} ObjType;

struct Obj {
    ObjType type;
    Obj*    next;
};

struct ObjString {
    Obj   obj;
    int   length;
    char* chars;
};

ObjString*
copy_string(const char* chars, int length);

void
print_object(Value value);

ObjString*
take_string(char* chars, int length);

static inline bool
is_obj_type(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
