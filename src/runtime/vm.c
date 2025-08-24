// File:    vm.c
// Purpose: implement vm.h
// Author:  Jake Hathaway
// Date:    2025-08-17

#include "vm.h"
#include "bytecode.h"
#include "compiler.h"
#include "debug.h"
#include "hash_map.h"
#include "memory.h"
#include "object.h"
#include "value.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

VM vm;

static Value
clock_native(int arg_count, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void
reset_stack() {
    vm.stack_top = vm.stack;
    vm.frame_count = 0;
    vm.open_upvalues = NULL;
}

static void
runtime_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frame_count - 1; i >= 0; i--) {
        CallFrame*   frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        size_t       instruction = frame->ip - function->bytecode.code - 1;
        fprintf(stderr, "[line %d] in ", function->bytecode.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    CallFrame* frame = &vm.frames[vm.frame_count - 1];
    size_t     instruction =
        frame->ip - frame->closure->function->bytecode.code - 1;
    int line = frame->closure->function->bytecode.lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack();
}

static void
define_native(const char* name, NativeFn function) {
    push(OBJ_VAL(copy_string(name, (int)strlen(name))));
    push(OBJ_VAL(new_native(function)));
    hashmap_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
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
call(ObjClosure* closure, int arg_count) {
    if (arg_count != closure->function->arity) {
        runtime_error(
            "Expected %d arguments but got %d.",
            closure->function->arity,
            arg_count);
        return false;
    }

    if (vm.frame_count == FRAMES_MAX) {
        runtime_error("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frame_count++];
    frame->closure = closure;
    frame->ip = closure->function->bytecode.code;
    frame->slots = vm.stack_top - arg_count - 1;
    return true;
}

static bool
call_value(Value callee, int arg_count) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), arg_count);
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value    result = native(arg_count, vm.stack_top - arg_count);
                vm.stack_top -= arg_count + 1;
                push(result);
                return true;
            }
            case OBJ_CLASS: {
                ObjClass* klass = AS_CLASS(callee);
                vm.stack_top[-arg_count - 1] = OBJ_VAL(new_instance(klass));
                Value initializer;
                if (hashmap_get(
                        &klass->methods, vm.init_string, &initializer)) {
                    return call(AS_CLOSURE(initializer), arg_count);
                } else if (arg_count != 0) {
                    runtime_error(
                        "Expected 0 arguments but got %d.", arg_count);
                    return false;
                }

                return true;
            }
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                vm.stack_top[-arg_count - 1] = bound->receiver;
                return call(bound->method, arg_count);
            }
            default:
                break; // Non-callable object type.
        }
    }
    runtime_error("Can only call functions and classes.");
    return false;
}

static bool
invoke_from_class(ObjClass* klass, ObjString* name, int arg_count) {
    Value method;
    if (!hashmap_get(&klass->methods, name, &method)) {
        runtime_error("Undefined property '%s'.", name->chars);
        return false;
    }

    return call(AS_CLOSURE(method), arg_count);
}

static bool
invoke(ObjString* name, int arg_count) {
    Value receiver = peek(arg_count);

    if (!IS_INSTANCE(receiver)) {
        runtime_error("Only instances have methods.");
        return false;
    }

    ObjInstance* instance = AS_INSTANCE(receiver);

    Value value;
    if (hashmap_get(&instance->fields, name, &value)) {
        vm.stack_top[-arg_count - 1] = value;
        return call_value(value, arg_count);
    }

    return invoke_from_class(instance->klass, name, arg_count);
}

static bool
bind_method(ObjClass* klass, ObjString* name) {
    Value method;
    if (!hashmap_get(&klass->methods, name, &method)) {
        runtime_error("Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod* bound = new_bound_method(peek(0), AS_CLOSURE(method));
    pop();
    push(OBJ_VAL(bound));
    return true;
}

static ObjUpvalue*
capture_upvalue(Value* local) {
    ObjUpvalue* prev_upvalue = NULL;
    ObjUpvalue* upvalue = vm.open_upvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prev_upvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* created_upvalue = new_upvalue(local);
    created_upvalue->next = upvalue;

    if (prev_upvalue == NULL) {
        vm.open_upvalues = created_upvalue;
    } else {
        prev_upvalue->next = created_upvalue;
    }

    return created_upvalue;
}

static void
close_upvalues(Value* last) {
    while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last) {
        ObjUpvalue* upvalue = vm.open_upvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.open_upvalues = upvalue->next;
    }
}

static void
define_method(ObjString* name) {
    Value     method = peek(0);
    ObjClass* klass = AS_CLASS(peek(1));
    hashmap_set(&klass->methods, name, method);
    pop();
}

static bool
is_falsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

// static void
// concatenate() {
//     const ObjString* b = AS_STRING(peek(0));
//     ObjString*       a = AS_STRING(peek(1));

//     int   length = a->length + b->length;
//     char* chars = ALLOCATE(char, length + 1);
//     memcpy(chars, a->chars, a->length);
//     memcpy(chars + a->length, b->chars, b->length);
//     chars[length] = '\0';

//     ObjString* result = take_string(chars, length);
//     pop();
//     pop();
//     push(OBJ_VAL(result));
// }

static void
concatenate() {
    Value b_val = peek(0);
    Value a_val = peek(1);

    // Convert numbers to strings if needed
    if (IS_NUMBER(a_val) || IS_NUMBER(b_val)) {
        ObjString* a_str = NULL;
        ObjString* b_str = NULL;

        // Convert first operand if it's a number
        if (IS_NUMBER(a_val)) {
            a_str = number_to_string(AS_NUMBER(a_val));
        } else {
            a_str = AS_STRING(a_val);
        }

        // Convert second operand if it's a number
        if (IS_NUMBER(b_val)) {
            b_str = number_to_string(AS_NUMBER(b_val));
        } else {
            b_str = AS_STRING(b_val);
        }

        // Concatenate the strings
        int   length = a_str->length + b_str->length;
        char* chars = ALLOCATE(char, length + 1);
        memcpy(chars, a_str->chars, a_str->length);
        memcpy(chars + a_str->length, b_str->chars, b_str->length);
        chars[length] = '\0';

        ObjString* result = take_string(chars, length);

        pop(); // pop b
        pop(); // pop a
        push(OBJ_VAL(result));
    } else if (IS_STRING(a_val) && IS_STRING(b_val)) {
        // Original string concatenation logic
        const ObjString* b = AS_STRING(peek(0));
        ObjString*       a = AS_STRING(peek(1));

        int   length = a->length + b->length;
        char* chars = ALLOCATE(char, length + 1);
        memcpy(chars, a->chars, a->length);
        memcpy(chars + a->length, b->chars, b->length);
        chars[length] = '\0';

        ObjString* result = take_string(chars, length);
        pop();
        pop();
        push(OBJ_VAL(result));
    } else {
        runtime_error("Operands must be two numbers or two strings.");
    }
}

void
init_vm() {
    reset_stack();
    vm.objects = NULL;

    vm.gray_count = 0;
    vm.gray_capacity = 0;
    vm.gray_stack = NULL;

    vm.bytes_allocated = 0;
    vm.next_gc = 1024 * 1024;

    init_hashmap(&vm.globals);
    init_hashmap(&vm.strings);

    vm.init_string = NULL;
    vm.init_string = copy_string("init", 4);

    define_native("clock", clock_native);
}

void
free_vm() {
    free_hashmap(&vm.globals);
    free_hashmap(&vm.strings);
    vm.init_string = NULL;
    free_objects();
}

static InterpretResult
run() {
    CallFrame* frame = &vm.frames[vm.frame_count - 1];

#define READ_WORD() (*frame->ip++)
#define READ_CONSTANT()                                                        \
    (frame->closure->function->bytecode.constants.values[READ_WORD()])
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
        disassemble_instruction(
            &frame->closure->function->bytecode,
            (int)(frame->ip - frame->closure->function->bytecode.code));
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
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint16_t slot = READ_WORD();
                frame->slots[slot] = peek(0);
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
            case OP_GET_UPVALUE: {
                uint16_t slot = READ_WORD();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint16_t slot = READ_WORD();
                *frame->closure->upvalues[slot]->location = peek(0);
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
                } else if (IS_NUMBER(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_STRING(peek(0)) && IS_NUMBER(peek(1))) {
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
            case OP_JUMP: {
                uint16_t offset = READ_WORD();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                // DANGER: potential source of errors
                uint16_t offset = READ_WORD();
                if (is_falsey(peek(0))) {
                    frame->ip += offset;
                }
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_WORD();
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {
                int arg_count = READ_WORD();
                if (!call_value(peek(arg_count), arg_count)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure*  closure = new_closure(function);
                push(OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalue_count; i++) {
                    uint16_t is_local = READ_WORD();
                    uint16_t index = READ_WORD();
                    if (is_local) {
                        closure->upvalues[i] =
                            capture_upvalue(frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_CLOSE_UPVALUE: {
                close_upvalues(vm.stack_top - 1);
                pop();
                break;
            }
            case OP_CLASS: {
                ObjString* name = READ_STRING();
                push(OBJ_VAL(name));
                ObjClass* klass = new_class(name);
                pop();
                push(OBJ_VAL(klass));
                break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(0))) {
                    runtime_error("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjInstance* instance = AS_INSTANCE(peek(0));
                ObjString*   name = READ_STRING();

                Value value;
                if (hashmap_get(&instance->fields, name, &value)) {
                    pop(); // the instance
                    push(value);
                    break;
                }

                if (!bind_method(instance->klass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(1))) {
                    runtime_error("Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjInstance* instance = AS_INSTANCE(peek(1));
                hashmap_set(&instance->fields, READ_STRING(), peek(0));
                Value value = pop();
                pop();
                push(value);
                break;
            }
            case OP_METHOD: {
                define_method(READ_STRING());
                break;
            }
            case OP_INVOKE: {
                ObjString* method = READ_STRING();
                int        arg_count = READ_WORD();
                if (!invoke(method, arg_count)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
            case OP_RETURN: {
                Value result = pop();
                close_upvalues(frame->slots);
                vm.frame_count--;
                if (vm.frame_count == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                vm.stack_top = frame->slots;
                push(result);
                frame = &vm.frames[vm.frame_count - 1];
                break;
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
    ObjFunction* function = compile(source);
    if (function == NULL)
        return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));
    ObjClosure* closure = new_closure(function);
    pop();
    push(OBJ_VAL(closure));
    call(closure, 0);
    return run();
}
