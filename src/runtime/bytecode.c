// File:    bytecode.c
// Purpose: bytecode.h implementation
// Author:  Jake Hathaway
// Date:    2025-08-17

#include "include/bytecode.h"
#include "include/memory.h"
#include "include/value.h"
#include <stdint.h>
#include <stdio.h>

void
init_bytecode(Bytecode* bytecode) {
    bytecode->count = 0;
    bytecode->capacity = 0;
    bytecode->code = NULL;
    bytecode->lines = NULL;
    init_value_array(&bytecode->constants);
}

void
free_bytecode(Bytecode* bytecode) {
    FREE_ARRAY(GlobalAllocator, uint16_t, bytecode->code, bytecode->capacity);
    FREE_ARRAY(GlobalAllocator, int, bytecode->lines, bytecode->capacity);
    free_value_array(&bytecode->constants);
    init_bytecode(bytecode);
}

void
write_bytecode(Bytecode* bytecode, uint16_t word, int line) {
    if (bytecode->capacity < bytecode->count + 1) {
        int old_capacity = bytecode->capacity;
        bytecode->capacity = GROW_CAPACITY(old_capacity);
        bytecode->code = GROW_ARRAY(
            GlobalAllocator,
            uint16_t,
            bytecode->code,
            old_capacity,
            bytecode->capacity);
        bytecode->lines = GROW_ARRAY(
            GlobalAllocator,
            int,
            bytecode->lines,
            old_capacity,
            bytecode->capacity);
    }

    bytecode->code[bytecode->count] = word;
    bytecode->lines[bytecode->count] = line;
    bytecode->count++;
}

int
write_constant(Bytecode* bytecode, Value value) {
    write_value_array(&bytecode->constants, value);
    return bytecode->constants.count - 1;
}
