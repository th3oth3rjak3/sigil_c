// File:    vm.h
// Purpose: Define the runtime virtual machine.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

#include "bytecode.h"
#include "memory.h"

#define STACK_MAX 256

/// The result of interpreting the bytecode.
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

/// The virtual machine executes the bytecode program.
typedef struct {
    Bytecode* bytecode;         // Compiled bytecode to execute.
    uint16_t* ip;               // The instruction pointer.
    Value     stack[STACK_MAX]; // The virtual machine stack.
    Value*    stack_top;        // The pointer to the top of the stack.
} VM;

/// Initialize the virtual machine.
///
/// Params:
/// - allocator: The allocator used to initialize the vm.
/// - vm: The virtual machine to initialize.
void
init_vm(Allocator allocator, VM* vm);

/// Free virtual machine resources.
///
/// Params:
/// - allocator: The allocator used to free.
/// - vm: The virtual machine to free.
void
free_vm(Allocator allocator, VM* vm);

/// Interpret the bytecode.
///
/// Params:
/// - bytecode: The bytecode to interpret.
///
/// Returns:
/// - InterpretResult: The result of intrpreting the bytecode.
InterpretResult
interpret(VM* vm, Bytecode* bytecode);

/// Push a new value to the top of the virtual machine stack.
///
/// Params:
/// - vm: The virtual machine.
/// _ value: The value to push onto the vm stack.
void
push(VM* vm, Value value);

/// Pop a value off of the virtual machine stack.
///
/// Params:
/// - vm: The virtual machine to pop the value from.
///
/// Returns:
/// - Value: The value popped off the stack.
Value
pop(VM* vm);
