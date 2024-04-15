#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"
#include "parser.h"

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
    return 0;
}
