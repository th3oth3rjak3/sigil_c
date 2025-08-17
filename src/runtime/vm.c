// File:    vm.c
// Purpose: implement vm.h
// Author:  Jake Hathaway
// Date:    2025-08-17

#include "include/vm.h"
#include "include/common.h"
#include "include/debug.h"
#include "include/value.h"

#include <stdio.h>

static void
reset_stack(VM* vm) {
    vm->stack_top = vm->stack;
}

void
push(VM* vm, Value value) {
    *vm->stack_top = value;
    vm->stack_top++;
}

Value
pop(VM* vm) {
    vm->stack_top--;
    return *vm->stack_top;
}

void
init_vm(Allocator allocator, VM* vm) {
    reset_stack(vm);
}

void
free_vm(Allocator allocator, VM* vm) {}

static InterpretResult
run(VM* vm) {
#define READ_WORD() (*vm->ip++)
#define READ_CONSTANT() (vm->bytecode->constants.values[READ_WORD()])
    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm->stack; slot < vm->stack_top; slot++) {
            printf("[ ");
            print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        disassemble_instruction(
            vm->bytecode, (int)(vm->ip - vm->bytecode->code));
#endif

        uint8_t instruction;
        switch (instruction = READ_WORD()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(vm, constant);
                break;
            }
            case OP_NEGATE: {
                push(vm, -pop(vm));
                break;
            }
            case OP_RETURN: {
                print_value(pop(vm));
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }

#undef READ_WORD
#undef READ_CONSTANT
}

InterpretResult
interpret(VM* vm, Bytecode* bytecode) {
    vm->bytecode = bytecode;
    vm->ip = vm->bytecode->code;

    return run(vm);
}
