// File:    common.h
// Purpose: Common types that will be referenced in many parts of the code.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

/// OpCode represents a runtime bytecode instruction.
typedef enum {
    OP_CONSTANT, // Load constant.
    OP_RETURN,   // Return from function call.
} OpCode;
