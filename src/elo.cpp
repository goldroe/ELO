#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "core.h"
#include "lexer.h"
#include "parser.h"
#include "ast_dump.h"
#include "semantic.h"

int main(int argc, char **argv) {
    argc--; argv++;
    if (argc == 0) {
        puts("ELO Compiler");
        puts("usage: ELO [option...] filename...");
        exit(0);
    }
    
    char *file_name = argv[0];

    bool do_ast_dump = false;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-astdump") == 0) {
            do_ast_dump = true;
        }
    }
    
    init_atom_map();
    init_keywords();

#if 0
        assert(make_atom("foo") == make_atom("foo"));
        assert(make_atom("buzz") == make_atom("buzz"));
        assert(make_atom("Entity") == make_atom("Entity"));
        assert(make_atom("obj") != make_atom("Obj"));
        assert(make_atom("foo") == make_atom("foo"));
#endif
    
    Parser parser(file_name);

    Sema_Analyzer sema_analyzer;
    sema_analyzer.init(&parser);
    sema_analyzer.resolve();

    if (do_ast_dump) {
        printf("\n");
        Ast_Dump ast_dump;
        ast_dump.dump(parser.root);
    }

    return 0;
}
