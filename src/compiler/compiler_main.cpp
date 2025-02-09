#include "base/base_core.h"
#include "base/base_arena.h"
#include "base/base_strings.h"
#define STB_SPRINTF_IMPLEMENTATION
#include <stb_sprintf.h>
#include "os/os.h"
#include "path/path.h"

#include "base/base_core.cpp"
#include "base/base_arena.cpp"
#include "base/base_strings.cpp"
#include "os/os.cpp"
#include "path/path.cpp"

#include "auto_array.h"

#include "lexer.h"
#include "report.h"
#include "atom.h"
#include "ast.h"
#include "types.h"
#include "parser.h"
#include "resolve.h"
#include "llvm_backend.h"

#include "report.cpp"
#include "atom.cpp"
#include "lexer.cpp"
#include "ast.cpp"
#include "types.cpp"
#include "parser.cpp"
#include "resolve.cpp"

#include "llvm_backend.cpp"

global Auto_Array<Source_File*> g_source_files;
global Arena *temp_arena;

internal void add_source_file(Source_File *file) {
    g_source_files.push(file);
}

internal void compiler_error(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 string = str8_pushfv(g_report_arena, fmt, args);
    va_end(args);
    printf("ELO: error: %s", (char *)string.data);
}

#ifdef OS_WINDOWS
global HANDLE hOut;
global DWORD dwOriginalOutMode;
#endif

bool _terminal_supports_ansi_colors;

internal void compiler_exit() {
#ifdef OS_WINDOWS
    if (hOut != INVALID_HANDLE_VALUE) {
        SetConsoleMode(hOut, dwOriginalOutMode);
    }
#endif
}

int main(int argc, char **argv) {
    argc--;
    argv++;

    if (argc == 0) {
        printf("ELO compiler.\n");
        printf("Usage: ELO option... filename...");
        return 1;
    }

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

    g_report_arena = arena_alloc(get_virtual_allocator(), KB(64));
    g_ast_arena = arena_alloc(get_virtual_allocator(), MB(8));

    atom_init();

    atom_keyword(TOKEN_NULL,     str8_lit("null"));
    atom_keyword(TOKEN_ENUM,     str8_lit("enum"));
    atom_keyword(TOKEN_STRUCT,   str8_lit("struct"));
    atom_keyword(TOKEN_TRUE,     str8_lit("true"));
    atom_keyword(TOKEN_FALSE,    str8_lit("false"));
    atom_keyword(TOKEN_IF,       str8_lit("if"));
    atom_keyword(TOKEN_WHILE,    str8_lit("while"));
    atom_keyword(TOKEN_FOR,      str8_lit("for"));
    atom_keyword(TOKEN_BREAK,    str8_lit("break"));
    atom_keyword(TOKEN_CONTINUE, str8_lit("continue"));
    atom_keyword(TOKEN_ELSE,     str8_lit("else"));
    atom_keyword(TOKEN_RETURN,   str8_lit("return"));
    atom_keyword(TOKEN_CAST,     str8_lit("cast"));
    atom_keyword(TOKEN_OPERATOR, str8_lit("operator"));
    atom_keyword(TOKEN_IN,       str8_lit("in"));

    atom_directive(TOKEN_LOAD,   str8_lit("#load"));
    atom_directive(TOKEN_IMPORT, str8_lit("#import"));

    register_builtin_types();

    temp_arena = arena_alloc(get_malloc_allocator(), KB(64));

    String8 file_name = str8_cstring(argv[0]);
    String8 file_path = path_join(temp_arena, os_current_dir(temp_arena), file_name);
    file_path = normalize_path(temp_arena, file_path);

    Lexer *lexer = new Lexer(file_path);

    Parser *parser = new Parser(lexer);
    parser->parse();

    Resolver *resolver = new Resolver(parser);
    resolver->resolve();

    int error_count = 0;

    //@Note Print reports
    for (int file_idx = 0; file_idx < g_source_files.count; file_idx++) {
        Source_File *file = g_source_files[file_idx];

        quick_sort(file->reports.data, Report*, file->reports.count, report_sort_compare);

        for (int report_idx = 0; report_idx < file->reports.count; report_idx++) {
            Report *report = file->reports[report_idx];
            print_report(report, file);

            if (report->kind == REPORT_PARSER_ERROR || report->kind == REPORT_AST_ERROR) error_count++;

            for (int i = 0; i < report->children.count; i++) {
                Report *child = report->children[i];
                print_report(child, file);
            }
        }

    }

    if (error_count == 0) {
        g_llvm_backend_arena = arena_alloc(get_virtual_allocator(), MB(2));
        llvm_backend(g_source_files[0], parser->root);
    }

    if (error_count > 0) {
        printf("%d error(s).\n", error_count);
    }

    compiler_exit();

    printf("done.\n");

    return 0;
}
