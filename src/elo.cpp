#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "lexer.h"
#include "parser.h"
#include "ast_dump.h"

int main(int argc, char **argv) {
    argc--; argv++;
    if (argc == 0) {
        puts("ELO Compiler");
        puts("usage: ELO [option...] filename...");
        exit(0);
    }
    
    char *file_name = argv[0];
    
    init_keywords();
    
    Parser parser(file_name);

    Ast_Dump ast_dump;
    ast_dump.dump(parser.root);
    return 0;
}
