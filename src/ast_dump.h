#ifndef AST_DUMP_H
#define AST_DUMP_H

#include <stdio.h>
#include <stdarg.h>

#include "parser.h"

class Ast_Dump {
    FILE *dump_file = nullptr;
    int spaces = 0;

    void print(const char *fmt, ...) {
        fprintf(dump_file, "%*.s", spaces, "");
        va_list args;
        va_start(args, fmt);
        vfprintf(dump_file, fmt, args);
        va_end(args);
    }
    void indent() {
        spaces += 2;
    }
    void outdent() {
        assert(spaces > 0);
        spaces -= 2;
    }

public:
    Ast_Dump() {
        dump_file = stdout;
    }
    Ast_Dump(const char *file_name) {
        dump_file = fopen(file_name, "w+");
        if (dump_file == nullptr) {
            printf("FAILED TO OPEN FILE\n");
        }
    }
    void dump(Ast *node);
    void dump_ast(Ast *root);
};

#endif // AST_DUMP_H
