#include "base/base_core.h"
#include "base/base_memory.h"
#include "base/base_strings.h"
#define STB_SPRINTF_IMPLEMENTATION
#include <stb_sprintf.h>

#include "string_map.cpp"

#include "os/os.h"
#include "path/path.h"

#include "base/base_core.cpp"
#include "base/base_memory.cpp"
#include "base/base_strings.cpp"
#include "os/os.cpp"
#include "path/path.cpp"

#include "auto_array.h"

#include "source_file.h"
#include "lexer.h"
#include "report.h"
#include "atom.h"
#include "ast.h"
#include "types.h"
#include "parser.h"
#include "resolve.h"
#include "llvm_backend.h"

global bool compiler_dump_IR;

#include "source_file.cpp"
#include "report.cpp"
#include "atom.cpp"
#include "lexer.cpp"
#include "types.cpp"
#include "ast.cpp"
#include "parser.cpp"
#include "resolve.cpp"
#include "llvm_backend.cpp"

internal void compiler_error(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 string = str8_pushfv(heap_allocator(), fmt, args);
    va_end(args);
    printf("ELO: error: %s", (char *)string.data);
    free(heap_allocator(), string.data);
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

internal void compiler_process_args(int argc, char **args) {
    for (int i = 0; i < argc; i++) {
        String8 arg = str8_cstring(args[i]);
        switch (arg.data[0]) {
        case '-':
        {
            if (arg.count == 0) {
                break;
            }

            if (str8_match(arg, str_lit("-dump_ir"), StringMatchFlag_CaseInsensitive)) {
                compiler_dump_IR = true;
            }
            break;
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
    llvm_arena = arena_create();

    atom_init();

    register_builtin_types();

    String8 file_name = str8_cstring(argv[0]);
    String8 file_path = path_join(heap_allocator(), os_current_dir(temporary_allocator()), file_name);
    file_path = normalize_path(heap_allocator(), file_path);

    String8 extension = path_get_extension(file_path);

    if (!os_file_exists(file_path)) {
        compiler_error("no such file or directory '%S'.\n", file_path);
        return 1;
    }
    if (!str8_equal(extension, str8_lit("elo"))) {
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
    parser->parse();

    Resolver *resolver = new Resolver(parser);
    resolver->resolve();

    //@Note Print reports
    for (Source_File *file = source_file_map.first; file; file = file->next) {
        quick_sort(file->reports.data, Report*, file->reports.count, report_sort_compare);

        for (int report_idx = 0; report_idx < file->reports.count; report_idx++) {
            Report *report = file->reports[report_idx];

#if !defined(BUILD_DEBUG)
            print_report(report, file);
            for (int i = 0; i < report->children.count; i++) {
                Report *child = report->children[i];
                print_report(child, file);
            }
#endif
        }
    }

    if (g_error_count > 0) {
        printf("%d error(s).\n", g_error_count);
    }

    if (g_error_count == 0) {
        LLVM_Backend *backend = new LLVM_Backend(source_file, parser->root);
        backend->gen();
    }

    compiler_exit();

    return g_error_count;
}
