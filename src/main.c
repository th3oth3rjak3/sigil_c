#include "debug.h"
#include "include/bytecode.h"
#include "include/memory.h"
#include "include/vm.h"

int
main(void) {
    Allocator alloc = get_debug_allocator();
    VM        vm;
    Bytecode  code;

    init_vm(alloc, &vm);
    init_bytecode(&code);

    int constant = write_constant(alloc, &code, 1.2);
    write_bytecode(alloc, &code, OP_CONSTANT, 123);
    write_bytecode(alloc, &code, constant, 123);
    write_bytecode(alloc, &code, OP_NEGATE, 123);

    write_bytecode(alloc, &code, OP_RETURN, 123);
    disassemble_bytecode(&code, "test bytecode");
    interpret(&vm, &code);

    free_vm(alloc, &vm);
    free_bytecode(alloc, &code);

    alloc.report_statistics();
}
