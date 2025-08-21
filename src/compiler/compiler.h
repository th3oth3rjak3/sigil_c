// File:    compiler.h
// Purpose: Define the compiler functions.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

#include "src/types/object.h"
#include <stdbool.h>

/// Compile the source code into a bytecode structure.
///
/// Params:
/// - source: The source code.
///
/// Returns:
/// - ObjFunction*: The compiled function.
ObjFunction*
compile(const char* source);
