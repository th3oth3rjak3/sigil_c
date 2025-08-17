// File:    debug.h
// Purpose: Code disassembly tools for development.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

#include "include/bytecode.h"

/// Disassemble and print the contents of the bytecode.
///
/// Params:
/// - bytecode: The bytecode struct to disassemble.
/// - name: The name of the section of code for display purposes.
void
disassemble_bytecode(Bytecode* bytecode, const char* name);

/// Disassemble a bytecode instruction.
///
/// Params:
/// - bytecode: The bytecode struct that contains the instruction.
/// - offset: The instruction offset for the instruction to disassemble.
///
/// Returns:
/// - int: The next instruction offset to disassemble.
int
disassemble_instruction(Bytecode* bytecode, int offset);
