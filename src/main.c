// File:    main.c
// Purpose: The main entry point for the sigil language interpreter.
// Author:  Jake Hathaway
// Date:    2025-08-17
#include "common.h"
#include "memory/memory.h"
#include "runtime/vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
repl(void) {
    char line[1024];

#ifdef _WIN32
    printf("Sigil REPL - Type 'exit' or press Ctrl+Z then Enter to quit\n");
#else
    printf("Sigil REPL - Type 'exit' or press Ctrl+D to quit\n");
#endif

    for (;;) {
        printf("> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        // Check for exit command (handle both "exit" and "exit\n")
        if (strncmp(line, "exit", 4) == 0) {
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
    size_t file_size = ftell(file);
    rewind(file);

    char* buffer = ALLOCATE(char, file_size + 1); // Fixed: was 'length + 1'
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytes_read] = '\0';
    fclose(file);
    *out_size = file_size + 1;
    return buffer;
}

static void
run_file(const char* path) {
    size_t          size;
    char*           source = read_file(path, &size);
    InterpretResult result = interpret(source);

    FREE_ARRAY(char, source, size);

    if (result == INTERPRET_COMPILE_ERROR) {
        exit(65);
    }
    if (result == INTERPRET_RUNTIME_ERROR) {
        exit(70);
    }
}

int
main(int argc, const char* argv[]) {
    init_vm();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        run_file(argv[1]);
    } else {
        fprintf(stderr, "Usage: sigil [path]\n");
        exit(64);
    }

    // Free first, then report
    free_vm();

#ifdef DEBUG_PRINT_ALLOCATIONS
    report_memory_statistics();
#endif

    return 0;
}