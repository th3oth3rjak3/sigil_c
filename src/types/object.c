#include <stdio.h>
#include <string.h>

#include "include/memory.h"
#include "include/object.h"
#include "include/value.h"
#include "include/vm.h"

#define ALLOCATE_OBJ(type, objectType)                                         \
    (type*)allocate_object(sizeof(type), objectType)

static Obj*
allocate_object(size_t size, ObjType type) {
    Obj* object = (Obj*)GlobalAllocator.realloc(NULL, 0, size);
    object->type = type;

    object->next = vm.objects;
    vm.objects = object;
    return object;
}

static ObjString*
allocate_string(char* chars, int length) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

ObjString*
copy_string(const char* chars, int length) {
    char* heapChars = ALLOC_WITH(GlobalAllocator, char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocate_string(heapChars, length);
}

ObjString*
take_string(char* chars, int length) {
    return allocate_string(chars, length);
}

void
print_object(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
}
