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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
typedef void (*ParseFn)(bool can_assign);

/// ParseRule contains parsing hierarchy for each token type.
typedef struct {
    ParseFn    prefix;     // A prefix function (eg, unary) or null
    ParseFn    infix;      // An infix function (eg, binary) or null
    Precedence precedence; // Parsing precedence
} ParseRule;

/// Local is a local variable.
typedef struct {
    Token name;  // The name of the variable.
    int   depth; // The scope depth.
} Local;

/// Represents various types of functions.
typedef enum {
    TYPE_FUNCTION, // A regular function.
    TYPE_SCRIPT    // A top-level function
} FunctionType;

/// State about the code being compiled currently.
typedef struct {
    struct Compiler* enclosing; // A parent compiler.
    ObjFunction*     function;  // The currently compiling function.
    FunctionType     type;      // The type of function being compiled.
    Local            locals[UINT16_COUNT]; // Local variable storage.
    int              local_count;          // How many locals are in scope.
    int              scope_depth; // How many blocks are surrounding this code.
} Compiler;

/// The global parser.
Parser parser;

/// The current global compiler.
Compiler* current = NULL;

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

static bool
check(TokenType type) {
    return parser.current.type == type;
}

static bool
match(TokenType type) {
    if (!check(type))
        return false;
    advance();
    return true;
}

static Bytecode*
current_bytecode() {
    return &current->function->bytecode;
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

static void
emit_loop(int loop_start) {
    emit_word(OP_LOOP);

    int offset = current_bytecode()->count - loop_start + 1;
    if (offset > UINT16_MAX)
        error("Loop body too large.");

    emit_word(offset & 0xffff);
}

static int
emit_jump(uint16_t instruction) {
    emit_word(instruction);
    emit_word(0xffff);
    // Diverges from the book, we only go back one since we only add one
    // additional word instruction instead of two byte instructions
    // DANGER: potential source of errors
    return current_bytecode()->count - 1;
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
patch_jump(int offset) {
    // diverges from book, only use -1 since the word is only one instruction.
    // DANGER: potential source of errors
    int jump = current_bytecode()->count - offset - 1;
    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    // DANGER: potential source of errors
    current_bytecode()->code[offset] = jump & 0xffff;
}

static void
init_compiler(Compiler* compiler, FunctionType type) {
    compiler->enclosing = (struct Compiler*)current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->function = new_function();
    current = compiler;

    if (type != TYPE_SCRIPT) {
        current->function->name =
            copy_string(parser.previous.start, parser.previous.length);
    }

    Local* local = &current->locals[current->local_count++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
}

static void
emit_return() {
    emit_word(OP_NIL);
    emit_word(OP_RETURN);
}

static ObjFunction*
end_compiler() {
    emit_return();
    ObjFunction* function = current->function;
#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error) {
        disassemble_bytecode(
            current_bytecode(),
            function->name != NULL ? function->name->chars : "<script>");
    }
#endif

    current = (Compiler*)current->enclosing;
    return function;
}

static void
begin_scope() {
    current->scope_depth++;
}

static void
end_scope() {
    current->scope_depth--;

    // while there are locals
    // and the last value's depth is greater than the current depth
    // pop off the value since we don't need it anymore.
    while (current->local_count > 0
           && current->locals[current->local_count - 1].depth
                  > current->scope_depth) {
        emit_word(OP_POP);
        current->local_count--;
    }
}

/* ========================== Forward Declarations ========================== */
static void
expression();

static void
statement();

static void
declaration();

static ParseRule*
get_rule(TokenType type);

static void
parse_precedence(Precedence precedence) {
    advance();
    ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
    if (prefix_rule == NULL) {
        error("Expect expression.");
        return;
    }

    bool can_assign = precedence <= PREC_ASSIGNMENT;
    prefix_rule(can_assign);

    while (precedence <= get_rule(parser.current.type)->precedence) {
        advance();
        ParseFn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule(can_assign);
    }

    if (can_assign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static uint16_t
identifier_constant(Token* name) {
    return make_constant(OBJ_VAL(copy_string(name->start, name->length)));
}

static bool
identifiers_equal(Token* a, Token* b) {
    if (a->length != b->length)
        return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int
resolve_local(Compiler* compiler, Token* name) {
    for (int i = compiler->local_count - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiers_equal(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static void
add_local(Token name) {
    if (current->local_count == UINT16_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->local_count++];
    local->name = name;
    local->depth = -1;
}

static void
declare_variable() {
    if (current->scope_depth == 0)
        return;

    Token* name = &parser.previous;

    // ensure users can't create duplicate variable declarations in the same
    // scope.
    for (int i = current->local_count - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        // If the current local in this iteration has a lower scope depth
        // than our current, we didn't find the variable in the current scope.
        if (local->depth != -1 && local->depth < current->scope_depth) {
            break;
        }

        // If the names are equal, a variable was already declared with this
        // name.
        if (identifiers_equal(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    add_local(*name);
}

static uint16_t
parse_variable(const char* error_message) {
    consume(TOKEN_IDENTIFIER, error_message);

    declare_variable();
    if (current->scope_depth > 0)
        return 0;

    return identifier_constant(&parser.previous);
}

static void
mark_initialized() {
    if (current->scope_depth == 0)
        return;

    current->locals[current->local_count - 1].depth = current->scope_depth;
}

static void
define_variable(uint16_t global) {
    if (current->scope_depth > 0) {
        mark_initialized();
        return;
    }

    emit_words(OP_DEFINE_GLOBAL, global);
}

static uint16_t
argument_list() {
    uint16_t arg_count = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (arg_count == 1000) {
                error("Can't have more than 1000 arguments.");
            }
            arg_count++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return arg_count;
}

static void
and_(bool can_assign) {
    int end_jump = emit_jump(OP_JUMP_IF_FALSE);

    emit_word(OP_POP);
    parse_precedence(PREC_AND);

    patch_jump(end_jump);
}

static void
expression() {
    parse_precedence(PREC_ASSIGNMENT);
}

static void
block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void
function(FunctionType type) {
    Compiler compiler;
    init_compiler(&compiler, type);
    begin_scope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                error_at_current("Can't have more than 255 parameters.");
            }
            uint16_t constant = parse_variable("Expect parameter name.");
            define_variable(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    ObjFunction* function = end_compiler();
    emit_words(OP_CONSTANT, make_constant(OBJ_VAL(function)));
}

static void
fun_declaration() {
    uint8_t global = parse_variable("Expect function name.");
    mark_initialized();
    function(TYPE_FUNCTION);
    define_variable(global);
}

static void
var_declaration() {
    uint16_t global = parse_variable("Expect variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emit_word(OP_NIL);
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    define_variable(global);
}

static void
expression_statement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emit_word(OP_POP);
}

static void
for_statement() {
    begin_scope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TOKEN_SEMICOLON)) {
        // no initializer present
    } else if (match(TOKEN_VAR)) {
        var_declaration();
    } else {
        expression_statement();
    }

    int loop_start = current_bytecode()->count;
    int exit_jump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // jump out if condition is false.
        exit_jump = emit_jump(OP_JUMP_IF_FALSE);
        emit_word(OP_POP); // remove the condition
    }

    if (!match(TOKEN_RIGHT_PAREN)) {
        int body_jump = emit_jump(OP_JUMP);
        int increment_start = current_bytecode()->count;
        expression();
        emit_word(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emit_loop(loop_start);
        loop_start = increment_start;
        patch_jump(body_jump);
    }

    statement();
    emit_loop(loop_start);

    if (exit_jump != -1) {
        patch_jump(exit_jump);
        emit_word(OP_POP);
    }

    end_scope();
}

static void
if_statement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int then_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_word(OP_POP);
    statement();

    int else_jump = emit_jump(OP_JUMP);

    patch_jump(then_jump);
    emit_word(OP_POP);

    if (match(TOKEN_ELSE)) {
        statement();
    }

    patch_jump(else_jump);
}

static void
print_statement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_word(OP_PRINT);
}

static void
return_statement() {
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }

    if (match(TOKEN_SEMICOLON)) {
        emit_return();
    } else {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emit_word(OP_RETURN);
    }
}

static void
while_statement() {
    int loop_start = current_bytecode()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_word(OP_POP);
    statement();
    emit_loop(loop_start);

    patch_jump(exit_jump);
    emit_word(OP_POP);
}

static void
synchronize() {
    parser.panic_mode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;

        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:; // Do nothing
        }

        advance();
    }
}

static void
statement() {
    if (match(TOKEN_PRINT)) {
        print_statement();
    } else if (match(TOKEN_IF)) {
        if_statement();
    } else if (match(TOKEN_RETURN)) {
        return_statement();
    } else if (match(TOKEN_WHILE)) {
        while_statement();
    } else if (match(TOKEN_FOR)) {
        for_statement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        begin_scope();
        block();
        end_scope();
    } else {
        expression_statement();
    }
}

static void
declaration() {
    if (match(TOKEN_FUN)) {
        fun_declaration();
    } else if (match(TOKEN_VAR)) {
        var_declaration();
    } else {
        statement();
    }

    if (parser.panic_mode)
        synchronize();
}

static void
grouping(bool can_assign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void
number(bool can_assign) {
    double value = strtod(parser.previous.start, NULL);
    emit_constant(NUMBER_VAL(value));
}

static void
or_(bool can_assign) {
    int else_jump = emit_jump(OP_JUMP_IF_FALSE);
    int end_jump = emit_jump(OP_JUMP);

    patch_jump(else_jump);
    emit_word(OP_POP);

    parse_precedence(PREC_OR);
    patch_jump(end_jump);
}

static void
string(bool can_assign) {
    emit_constant(OBJ_VAL(
        copy_string(parser.previous.start + 1, parser.previous.length - 2)));
}

static void
named_variable(Token name, bool can_assign) {
    uint16_t get_op, set_op;
    int      arg = resolve_local(current, &name);

    if (arg != -1) {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    } else {
        arg = identifier_constant(&name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }

    if (can_assign && match(TOKEN_EQUAL)) {
        expression();
        emit_words(set_op, (uint16_t)arg);
    } else {
        emit_words(get_op, (uint16_t)arg);
    }
}

static void
variable(bool can_assign) {
    named_variable(parser.previous, can_assign);
}

static void
unary(bool can_assign) {
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
binary(bool can_assign) {
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
call(bool can_assign) {
    uint16_t arg_count = argument_list();
    emit_words(OP_CALL, arg_count);
}

static void
literal(bool can_assign) {
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
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
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
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
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

ObjFunction*
compile(const char* source) {
    init_scanner(source);
    Compiler compiler;
    init_compiler(&compiler, TYPE_SCRIPT);

    parser.had_error = false;
    parser.panic_mode = false;

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    ObjFunction* function = end_compiler();
    return parser.had_error ? NULL : function;
}
