#define ANSI_RESET     "\x1B[0m"
#define ANSI_UNDERLINE "\x1B[4m"
#define ANSI_ITALIC    "\x1B[3m"

global Arena *g_report_arena;
global int g_error_count;

internal u64 get_line_start_after_spaces(String8 string, u64 start) {
    for (u64 i = start; i < string.count; i++) {
        if (string.data[i] == '\n' || string.data[i] == '\r') {
            return start;
        }
        if (!isspace(string.data[i])) {
            return i;
        }
    }
    return start;
}

internal u64 get_next_line_boundary(String8 string, u64 start) {
    for (u64 i = start; i < string.count; i++) {
        if (string.data[i] == '\n' || string.data[i] == '\r') {
            return i;
        }
    }
    return string.count - 1;
} 

internal void report_ast_error_va(Source_Pos start, Source_Pos end, const char *fmt, va_list va) {
    Source_File *file = start.file;
    String8 buffer = file->text;
    u64 line_start = get_line_start_after_spaces(buffer, start.index - start.col);
    u64 line_end = get_next_line_boundary(buffer, start.index);
    u64 end_index = Min(end.index, line_end);

    printf("%s:%llu:%llu: error: ", file->path.data, start.line, start.col);
    vprintf(fmt, va);

    printf("\t");

    if (line_start < start.index) {
        printf("\x1B[38;2;168;153;132m");
        String8 pre_string = str8(buffer.data + line_start, start.index - line_start);
        printf("%.*s", (int)pre_string.count, pre_string.data);
        printf(ANSI_RESET);
    }

    String8 elem_string = str8(buffer.data + start.index, end_index - start.index);
    printf("\x1B[38;2;204;36;29m");
    printf("%.*s", (int)elem_string.count, elem_string.data);
    printf(ANSI_RESET);

    if (line_end > end_index) {
        String8 trailing = str8(buffer.data + end_index, line_end - end_index);
        printf("\x1B[38;2;168;153;132m");
        printf("%.*s", (int)trailing.count, trailing.data);
        printf(ANSI_RESET);
    }

    if (end.index > line_end) {
        printf(ANSI_ITALIC "\x1B[38;2;168;153;132m");
        printf("...");
        printf(ANSI_RESET);
    }
    printf("\n");
}

internal void report_ast_error(Ast *node, const char *fmt, ...) {
    Source_Pos pos = node->start;
    Source_Pos end = node->end;
    va_list args;
    va_start(args, fmt);
    report_ast_error_va(pos, end, fmt, args);
    va_end(args);
    g_error_count++;
}

void report_undeclared(Ast_Ident *ident) {
    report_ast_error(ident, "undeclared identifier '%s'.\n", ident->name->data);
}

void report_redeclaration(Ast_Decl *decl) {
    report_ast_error(decl, "redeclaration of '%s'.\n", decl->name->data);
}

internal void report_parser_error_va(Source_Pos pos, const char *fmt, va_list va) {
    printf("%s:%llu:%llu: syntax error: ", pos.file->path.data, pos.line, pos.col);
    vprintf(fmt, va);
}

internal void report_parser_error(Lexer *lexer, const char *fmt, ...) {
    Source_Pos pos = make_source_pos(lexer->source_file, lexer->line_number, lexer->column_number, lexer->stream_index);
    va_list args;
    va_start(args, fmt);
    report_parser_error_va(pos, fmt, args);
    va_end(args);
    g_error_count++;
}

internal void report_note_va(Source_Pos pos, const char *fmt, va_list va) {
    printf("%s:%llu:%llu: note: ", pos.file->path.data, pos.line, pos.col);
    vprintf(fmt, va);
}

internal void report_note(Source_Pos pos, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    report_note_va(pos, fmt, args);
    va_end(args);
}

internal void report_line(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}
