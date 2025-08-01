#include <libtommath/tommath.h>

#include "base/base_core.h"
#include "base/base_memory.h"
#include "base/base_strings.h"

#include <string_map.h>

#define STB_SPRINTF_IMPLEMENTATION
#include <stb_sprintf.h>

#include "os/os.h"
#include "path/path.h"

#include "array.h"
#include "source_file.h"
#include "constant_value.h"
#include "lexer.h"
#include "report.h"
#include "atom.h"
#include "ast.h"
#include "types.h"
#include "parser.h"
#include "decl.h"
#include "resolve.h"
#include "llvm_backend.h"

bool compiler_dump_IR;
Array<String> compiler_link_libraries;

void compiler_error(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String string = str8_pushfv(heap_allocator(), fmt, args);
    va_end(args);
    printf("ELO: error: %s", (char *)string.data);
    free(heap_allocator(), string.data);
}

#ifdef OS_WINDOWS
HANDLE hOut;
DWORD dwOriginalOutMode;
#endif

bool _terminal_supports_ansi_colors;

void compiler_exit() {
#ifdef OS_WINDOWS
    if (hOut != INVALID_HANDLE_VALUE) {
        SetConsoleMode(hOut, dwOriginalOutMode);
    }
#endif
}

void compiler_process_args(int argc, char **args) {
    array_init(&compiler_link_libraries, heap_allocator());
    for (int i = 0; i < argc; i++) {
        String arg = str8_cstring(args[i]);
        if (arg.count < 2) continue;

        if (arg.data[0] == '-') {
            arg.data++; arg.count--;
            if (arg.data[0] == 'l') {
                String lib = arg;
                lib.count--; lib.data++;
                array_add(&compiler_link_libraries, lib);
            } else if (str8_match(arg, str_lit("dump_ir"), StringMatchFlag_CaseInsensitive)) {
                compiler_dump_IR = true;
            }
        }
    }
}

int main(int argc, char **argv) {
    argc--; argv++;
    if (argc == 0) {
        printf("ELO compiler.\n");
        printf("Usage: ELO option... filename...");
        return 1;
    }

    compiler_process_args(argc, argv);

#ifdef OS_WINDOWS
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    dwOriginalOutMode = 0;
    if (hOut != INVALID_HANDLE_VALUE) {
        GetConsoleMode(hOut, &dwOriginalOutMode);
        DWORD dwRequestedOutModes = ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        DWORD dwOutMode = dwOriginalOutMode | dwRequestedOutModes;
        if (SetConsoleMode(hOut, dwOutMode)) {
            _terminal_supports_ansi_colors = true;
        }
    }
#endif

    temporary_arena = arena_create();
    g_report_arena = arena_create();
    g_ast_arena = arena_create();

    atom_init();

    register_builtin_types();

    String file_name = str8_cstring(argv[0]);
    String file_path = path_join(heap_allocator(), os_current_dir(temporary_allocator()), file_name);
    file_path = normalize_path(heap_allocator(), file_path);

    String extension = path_get_extension(file_path);

    if (!os_file_exists(file_path)) {
        compiler_error("no such file or directory '%S'.\n", file_path);
        return 1;
    }
    if (!str8_equal(extension, str_lit("elo"))) {
        compiler_error("'%S' is not a valid elo file.\n", file_path);
        return 1;
    }

    Source_File *source_file = source_file_create(file_path);
    add_source_file(source_file);
    if (!source_file) {
        compiler_error("Could not read file '%S'\n", file_path);
        return 1;
    }

    Lexer *lexer = new Lexer(source_file);

    Parser *parser = new Parser(lexer);
    parser->file = source_file;
    parser->parse();

    Resolver *resolver = new Resolver(parser);
    resolver->resolve();

    if (g_error_count > 0) {
        printf("%d error(s).\n", g_error_count);
    }

    llvm_arena = arena_create();
    if (g_error_count == 0) {
        LLVM_Backend *backend = new LLVM_Backend(source_file, parser->root);
        backend->gen();
    }

    compiler_exit();

    return g_error_count;
}
