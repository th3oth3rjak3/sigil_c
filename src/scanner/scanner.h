// File:    scanner.h
// Purpose: Define scanning functions for producing tokens.
// Author:  Jake Hathaway
// Date:    2025-08-17

#pragma once

typedef enum {
    // Single-character tokens.
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_SLASH,
    TOKEN_STAR,
    // One or two character tokens.
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    // Literals.
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    // Keywords.
    TOKEN_AND,
    TOKEN_CLASS,
    TOKEN_ELSE,
    TOKEN_FALSE,
    TOKEN_FOR,
    TOKEN_FUN,
    TOKEN_IF,
    TOKEN_NIL,
    TOKEN_OR,
    TOKEN_PRINT,
    TOKEN_RETURN,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_TRUE,
    TOKEN_VAR,
    TOKEN_WHILE,

    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;

/// A scanned token that represents a portion of the source code.
typedef struct {
    TokenType   type;   // The kind of token.
    const char* start;  // The lexeme start.
    int         length; // The length of the lexeme.
    int         line;   // The line number of the source code.
} Token;

/// Read a single token from the source code.
///
/// Returns:
/// - Token: The next token from the source code.
Token
scan_token(void);

/// Initialize the scanner.
///
/// Params:
/// - scanner: The scanner to initialize.
/// - source: The source code to scan.
void
init_scanner(const char* source);
