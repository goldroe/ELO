#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "ast_dump.h"

Atom *make_atom(const char *str, size_t len);
Atom *make_atom(const char *str);
int main(int argc, char **argv) {
    argc--; argv++;
    if (argc == 0) {
        puts("ELO Compiler");
        puts("usage: ELO [option...] filename...");
        exit(0);
    }
    
    char *file_name = argv[0];
    
    init_keywords();
    init_atom_map();

    {
        assert(make_atom("foo") == make_atom("foo"));
        assert(make_atom("buzz") == make_atom("buzz"));
        assert(make_atom("Entity") == make_atom("Entity"));
        assert(make_atom("obj") != make_atom("Obj"));
    }
    
    Parser parser(file_name);

    Ast_Dump ast_dump;
    ast_dump.dump(parser.root);
    return 0;
}
