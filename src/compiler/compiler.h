// File:    compiler.h
// Purpose: Define the compiler functions.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

#include "src/runtime/bytecode.h"
#include <stdbool.h>

/// Compile the source code into a bytecode structure.
///
/// Params:
/// - source: The source code.
/// - bytecode: An output param that contains the bytecode.
///
/// Returns:
/// - bool: True when compilation succeeds, otherwise false.
bool
compile(const char* source, Bytecode* bytecode);
