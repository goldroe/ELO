#ifndef AST_DUMP_H
#define AST_DUMP_H

#include <stdio.h>
#include <stdarg.h>

#include "parser.h"

class Ast_Dump {
    int spaces = 0;

    void print(const char *fmt, ...) {
        printf("%*.s", spaces, "");
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
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
    void dump(Ast *node);
};

#endif // AST_DUMP_H
