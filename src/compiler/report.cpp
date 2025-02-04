global Arena *g_report_arena;

internal Report *submit_report(Source_File *file, Report_Kind kind, String8 message, Source_Pos pos) {
    Report *result = push_array(g_report_arena, Report, 1);
    result->kind = kind;
    result->message = message;
    result->source_pos = pos; 
    file->reports.push(result);
    return result;
} 

internal void report_parser_error(Lexer *lexer, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 message = str8_pushfv(g_report_arena, fmt, args);
    va_end(args);

    Source_Pos pos = { lexer->column_number, lexer->line_number, lexer->stream_index, lexer->source_file };
    Source_File *file = lexer->source_file;

    Report *report = submit_report(file, REPORT_PARSER_ERROR, message, pos);
}

internal void report_ast_error(Ast *node, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 message = str8_pushfv(g_report_arena, fmt, args);
    va_end(args);

    Source_File *file = node->start.file;

    Report *report = submit_report(file, REPORT_AST_ERROR, message, node->start);
    report->node = node;
}

internal int report_sort_compare(const void *a, const void *b) {
    Report *first  = *(Report**)a;
    Report *second = *(Report**)b;

    bool fp = first->kind == REPORT_PARSER_ERROR;
    bool sp = second->kind == REPORT_PARSER_ERROR;

    if (fp && !sp) {
        return -1;
    } else if (!fp && sp) {
        return 1;
    }

    u64 l0 = first->source_pos.line;
    u64 l1 = second->source_pos.line;
    if (l0 < l1) {
        return -1;
    } else if (l0 > l1) {
        return 1;
    } else {
        u64 c0 = first->source_pos.col;
        u64 c1 = second->source_pos.col;
        if (c0 < c1) {
            return -1;
        } else if (c0 > c1) {
            return 1;
        }
    }
    return 0;
}
