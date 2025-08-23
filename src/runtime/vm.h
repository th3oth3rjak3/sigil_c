// File:    vm.h
// Purpose: Define the runtime virtual machine.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

#include "hash_map.h"
#include <stddef.h>

#define FRAMES_MAX 1000
#define STACK_MAX (FRAMES_MAX * 1024)

/// A function call frame for managing function state.
typedef struct {
    ObjClosure* closure; // The function closure for the call frame.
    uint16_t*   ip;      // The instruction pointer to return to.
    Value*      slots;   // A pointer to local value slots.
} CallFrame;

/// The result of interpreting the bytecode.
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

/// The virtual machine executes the bytecode program.
typedef struct {
    CallFrame   frames[FRAMES_MAX]; // A list of call frames.
    int         frame_count;        // The number of call frames used.
    Value       stack[STACK_MAX];   // The virtual machine stack.
    Value*      stack_top;          // The pointer to the top of the stack.
    Obj*        objects;         // The list of allocated objects on the heap.
    HashMap     strings;         // The collection of interned strings.
    ObjUpvalue* open_upvalues;   // Upvalues that are still live in the stack.
    HashMap     globals;         // The collection of global variables.
    int         gray_count;      // The number of gray objects.
    int         gray_capacity;   // The total amount of capacity.
    Obj**       gray_stack;      // The gc worklist.
    size_t      bytes_allocated; // Size of heap allocations by gc
    size_t      next_gc;         // Threshold for next gc in bytes
} VM;

extern VM vm;

/// Initialize the virtual machine.
void
init_vm();

/// Free virtual machine resources.
void
free_vm();

/// Interpret the bytecode.
///
/// Params:
/// - source: The source code to interpret.
///
/// Returns:
/// - InterpretResult: The result of intrpreting the bytecode.
InterpretResult
interpret(const char* source);

/// Push a new value to the top of the virtual machine stack.
///
/// Params:
/// _ value: The value to push onto the vm stack.
void
push(Value value);

/// Pop a value off of the virtual machine stack.
///
/// Returns:
/// - Value: The value popped off the stack.
Value
pop();
