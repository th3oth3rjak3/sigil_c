// File:    debug.c
// Purpose: implementation of debug.h
// Author:  Jake Hathaway
// Date:    2025-08-17

#include "include/debug.h"
#include "include/bytecode.h"

#include <stdio.h>

static int
simple_instruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int
constant_instruction(const char* name, Bytecode* bytecode, int offset) {
    uint8_t constant = bytecode->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    print_value(bytecode->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

void
disassemble_bytecode(Bytecode* bytecode, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < bytecode->count;) {
        offset = disassemble_instruction(bytecode, offset);
    }
}

int
disassemble_instruction(Bytecode* bytecode, int offset) {
    printf("%04d ", offset);
    if (offset > 0 && bytecode->lines[offset] == bytecode->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", bytecode->lines[offset]);
    }
    uint8_t instruction = bytecode->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constant_instruction("OP_CONSTANT", bytecode, offset);
        case OP_ADD:
            return simple_instruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simple_instruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simple_instruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simple_instruction("OP_DIVIDE", offset);
        case OP_NEGATE:
            return simple_instruction("OP_NEGATE", offset);
        case OP_RETURN:
            return simple_instruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
