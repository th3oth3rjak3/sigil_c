// File:    vm.c
// Purpose: implement vm.h
// Author:  Jake Hathaway
// Date:    2025-08-17

#include "include/vm.h"
#include "include/common.h"
#include "include/compiler.h"
#include "include/debug.h"
#include "include/value.h"

#include <stdbool.h>
#include <stdio.h>

VM vm;

static void
reset_stack() {
    vm.stack_top = vm.stack;
}

void
push(Value value) {
    *vm.stack_top = value;
    vm.stack_top++;
}

Value
pop() {
    vm.stack_top--;
    return *vm.stack_top;
}

void
init_vm() {
    reset_stack();
}

void
free_vm() {}

static InterpretResult
run() {
#define READ_WORD() (*vm.ip++)
#define READ_CONSTANT() (vm.bytecode->constants.values[READ_WORD()])
#define BINARY_OP(op)                                                          \
    do {                                                                       \
        double b = pop();                                                      \
        double a = pop();                                                      \
        push(a op b);                                                          \
    } while (false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
            printf("[ ");
            print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        disassemble_instruction(vm.bytecode, (int)(vm.ip - vm.bytecode->code));
#endif

        uint8_t instruction;
        switch (instruction = READ_WORD()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_ADD:
                BINARY_OP(+);
                break;
            case OP_SUBTRACT:
                BINARY_OP(-);
                break;
            case OP_MULTIPLY:
                BINARY_OP(*);
                break;
            case OP_DIVIDE:
                BINARY_OP(/);
                break;
            case OP_NEGATE: {
                push(-pop());
                break;
            }
            case OP_RETURN: {
                print_value(pop());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }

#undef READ_WORD
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult
interpret(const char* source) {
    Bytecode bytecode;
    init_bytecode(&bytecode);

    if (!compile(source, &bytecode)) {
        free_bytecode(&bytecode);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.bytecode = &bytecode;
    vm.ip = vm.bytecode->code;

    InterpretResult result = run();

    free_bytecode(&bytecode);
    return result;
}
