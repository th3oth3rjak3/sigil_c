// File:    vm.c
// Purpose: implement vm.h
// Author:  Jake Hathaway
// Date:    2025-08-17

#include "include/vm.h"
#include "include/common.h"
#include "include/compiler.h"
#include "include/debug.h"
#include "include/value.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

VM vm;

static void
reset_stack() {
    vm.stack_top = vm.stack;
}

static void
runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.bytecode->code - 1;
    int    line = vm.bytecode->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack();
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

static Value
peek(int distance) {
    return vm.stack_top[-1 - distance];
}

static bool
is_falsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
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
#define BINARY_OP(valueType, op)                                               \
    do {                                                                       \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                      \
            runtimeError("Operands must be numbers.");                         \
            return INTERPRET_RUNTIME_ERROR;                                    \
        }                                                                      \
        double b = AS_NUMBER(pop());                                           \
        double a = AS_NUMBER(pop());                                           \
        push(valueType(a op b));                                               \
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
            case OP_NIL:
                push(NIL_VAL);
                break;
            case OP_TRUE:
                push(BOOL_VAL(true));
                break;
            case OP_FALSE:
                push(BOOL_VAL(false));
                break;
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(values_equal(a, b)));
                break;
            }
            case OP_GREATER:
                BINARY_OP(BOOL_VAL, >);
                break;
            case OP_LESS:
                BINARY_OP(BOOL_VAL, <);
                break;
            case OP_ADD:
                BINARY_OP(NUMBER_VAL, +);
                break;
            case OP_SUBTRACT:
                BINARY_OP(NUMBER_VAL, -);
                break;
            case OP_MULTIPLY:
                BINARY_OP(NUMBER_VAL, *);
                break;
            case OP_DIVIDE:
                BINARY_OP(NUMBER_VAL, /);
                break;
            case OP_NOT:
                push(BOOL_VAL(is_falsey(pop())));
                break;
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
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
