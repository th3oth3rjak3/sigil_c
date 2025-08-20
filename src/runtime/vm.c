// File:    vm.c
// Purpose: implement vm.h
// Author:  Jake Hathaway
// Date:    2025-08-17

#include "src/runtime/vm.h"
#include "src/common.h"
#include "src/compiler/compiler.h"
#include "src/debug/debug.h"
#include "src/memory/memory.h"
#include "src/runtime/bytecode.h"
#include "src/types/hash_map.h"
#include "src/types/object.h"
#include "src/types/value.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

VM vm;

static void
reset_stack() {
    vm.stack_top = vm.stack;
}

static void
runtime_error(const char* format, ...) {
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

static void
concatenate() {
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int   length = a->length + b->length;
    char* chars = ALLOC_WITH(GlobalAllocator, char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = take_string(chars, length);
    push(OBJ_VAL(result));
}

void
init_vm() {
    reset_stack();
    vm.objects = NULL;
    init_hashmap(&vm.globals);
    init_hashmap(&vm.strings);
}

void
free_vm() {
    free_hashmap(&vm.globals);
    free_hashmap(&vm.strings);
    free_objects();
}

static InterpretResult
run() {
#define READ_WORD() (*vm.ip++)
#define READ_CONSTANT() (vm.bytecode->constants.values[READ_WORD()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(value_type, op)                                              \
    do {                                                                       \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                      \
            runtime_error("Operands must be numbers.");                        \
            return INTERPRET_RUNTIME_ERROR;                                    \
        }                                                                      \
        double b = AS_NUMBER(pop());                                           \
        double a = AS_NUMBER(pop());                                           \
        push(value_type(a op b));                                              \
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
            case OP_POP:
                pop();
                break;
            case OP_GET_LOCAL: {
                uint16_t slot = READ_WORD();
                push(vm.stack[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint16_t slot = READ_WORD();
                vm.stack[slot] = peek(0);
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value      value;
                if (!hashmap_get(&vm.globals, name, &value)) {
                    runtime_error("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                hashmap_set(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                if (hashmap_set(&vm.globals, name, peek(0))) {
                    hashmap_delete(&vm.globals, name);
                    runtime_error("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
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
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtime_error(
                        "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
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
                    runtime_error("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }
            case OP_PRINT: {
                print_value(pop());
                printf("\n");
                break;
            }
            case OP_RETURN: {
                return INTERPRET_OK;
            }
        }
    }

#undef READ_WORD
#undef READ_CONSTANT
#undef READ_STRING
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
