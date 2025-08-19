// File:    compiler.c
// Purpose: implement compiler.h
// Author:  Jake Hathaway
// Date:    2025-08-17

#include "src/common.h"
#include "src/runtime/bytecode.h"
#include "src/scanner/scanner.h"
#include "src/types/object.h"
#include "src/types/value.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PRINT_CODE
#include "src/debug/debug.h"
#endif

/// Parser handles precedence parsing of tokens into bytecode.
typedef struct {
    Token current;    // The current token.
    Token previous;   // The previous token.
    bool  had_error;  // True when an error has occurred during parsing.
    bool  panic_mode; // True when syncrhonization is necessary.
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} Precedence;

/// A function that is used to parse tokens.
typedef void (*ParseFn)();

/// ParseRule contains parsing hierarchy for each token type.
typedef struct {
    ParseFn    prefix;     // A prefix function (eg, unary) or null
    ParseFn    infix;      // An infix function (eg, binary) or null
    Precedence precedence; // Parsing precedence
} ParseRule;

/// The global parser.
Parser parser;

/// The currently compiling bytecode segment.
Bytecode* compiling_bytecode;

static void
error_at(Token* token, const char* message) {
    if (parser.panic_mode)
        return;

    parser.panic_mode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.had_error = true;
}

static void
error(const char* message) {
    error_at(&parser.previous, message);
}

static void
error_at_current(const char* message) {
    error_at(&parser.current, message);
}

static void
advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scan_token();
        if (parser.current.type != TOKEN_ERROR)
            break;

        error_at_current(parser.current.start);
    }
}

static void
consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    error_at_current(message);
}

static Bytecode*
current_bytecode() {
    return compiling_bytecode;
}

static void
emit_word(uint16_t word) {
    write_bytecode(current_bytecode(), word, parser.previous.line);
}

static void
emit_words(uint16_t word1, uint16_t word2) {
    emit_word(word1);
    emit_word(word2);
}

static uint16_t
make_constant(Value value) {
    int constant = write_constant(current_bytecode(), value);
    if (constant > UINT16_MAX) {
        error("Too many constants in one bytecode array.");
        return 0;
    }

    return (uint16_t)constant;
}

static void
emit_constant(Value value) {
    emit_words(OP_CONSTANT, make_constant(value));
}

static void
emit_return() {
    emit_word(OP_RETURN);
}

static void
end_compiler() {
    emit_return();
#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error) {
        disassemble_bytecode(current_bytecode(), "code");
    }
#endif
}

static void
expression();

static ParseRule*
get_rule(TokenType type);

static void
parse_precedence(Precedence precedence);

static void
parse_precedence(Precedence precedence) {
    advance();
    ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
    if (prefix_rule == NULL) {
        error("Expect expression.");
        return;
    }

    prefix_rule();

    while (precedence <= get_rule(parser.current.type)->precedence) {
        advance();
        ParseFn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule();
    }
}

static void
expression() {
    parse_precedence(PREC_ASSIGNMENT);
}

static void
grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void
number() {
    double value = strtod(parser.previous.start, NULL);
    emit_constant(NUMBER_VAL(value));
}

static void
string() {
    emit_constant(OBJ_VAL(
        copy_string(parser.previous.start + 1, parser.previous.length - 2)));
}

static void
unary() {
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    parse_precedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_MINUS:
            emit_word(OP_NEGATE);
            break;
        case TOKEN_BANG:
            emit_word(OP_NOT);
            break;
        default:
            return; // Unreachable.
    }
}

static void
binary() {
    TokenType  operator_type = parser.previous.type;
    ParseRule* rule = get_rule(operator_type);
    parse_precedence((Precedence)(rule->precedence + 1));

    switch (operator_type) {
        case TOKEN_PLUS:
            emit_word(OP_ADD);
            break;
        case TOKEN_MINUS:
            emit_word(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emit_word(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emit_word(OP_DIVIDE);
            break;
        case TOKEN_BANG_EQUAL:
            emit_words(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emit_word(OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emit_word(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emit_words(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emit_word(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emit_words(OP_GREATER, OP_NOT);
            break;
        default:
            return; // Unreachable.
    }
}

static void
literal() {
    switch (parser.previous.type) {
        case TOKEN_FALSE:
            emit_word(OP_FALSE);
            break;
        case TOKEN_NIL:
            emit_word(OP_NIL);
            break;
        case TOKEN_TRUE:
            emit_word(OP_TRUE);
            break;
        default:
            return; // Unreachable.
    }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static ParseRule*
get_rule(TokenType type) {
    return &rules[type];
}

bool
compile(const char* source, Bytecode* bytecode) {
    init_scanner(source);
    compiling_bytecode = bytecode;

    parser.had_error = false;
    parser.panic_mode = false;

    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    end_compiler();
    return !parser.had_error;
}
