#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"

int main(int argc, char **argv) {
    argc--; argv++;
    if (argc == 0) {
        puts("ELO Compiler");
        puts("usage: ELO [option...] filename...");
        exit(0);
    }

    Lexer lexer(argv[0]);

    while (lexer.token.type != TOKEN_EOF) 
    {
        lexer.scan();
    }
    return 0;
}
