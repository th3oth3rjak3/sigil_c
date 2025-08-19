// File:    main.c
// Purpose: The main entry point for the sigil language interpreter.
// Author:  Jake Hathaway
// Date:    2025-08-17

#include "src/memory/memory.h"
#include "src/runtime/vm.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
repl() {
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

static char*
read_file(const char* path, size_t* out_size) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);

    rewind(file);

    char* buffer = (char*)GlobalAllocator.alloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytesRead] = '\0';

    fclose(file);

    *out_size = fileSize + 1;
    return buffer;
}

static void
run_file(const char* path) {
    size_t          size;
    char*           source = read_file(path, &size);
    InterpretResult result = interpret(source);
    GlobalAllocator.free(source, size);

    if (result == INTERPRET_COMPILE_ERROR)
        exit(65);
    if (result == INTERPRET_RUNTIME_ERROR)
        exit(70);
}

void
handle_sigint(int sig) {
    free_vm();
    GlobalAllocator.report_statistics();
    exit(0);
}

int
main(int argc, const char* argv[]) {
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    init_vm();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        run_file(argv[1]);
    } else {
        fprintf(stderr, "Usage: sigil [path]\n");
        exit(64);
    }

    free_vm();
}
