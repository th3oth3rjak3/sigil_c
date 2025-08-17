#include "debug.h"
#include "include/bytecode.h"
#include "include/common.h"
#include "include/memory.h"

int
main(void) {
    Allocator alloc = get_debug_allocator();
    Bytecode  code;

    init_bytecode(&code);

    int constant = write_constant(alloc, &code, 1.2);
    write_bytecode(alloc, &code, OP_CONSTANT, 123);
    write_bytecode(alloc, &code, constant, 123);

    write_bytecode(alloc, &code, OP_RETURN, 123);
    disassemble_bytecode(&code, "test bytecode");
    free_bytecode(alloc, &code);

    alloc.report_statistics();
}
