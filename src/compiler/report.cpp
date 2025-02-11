global Arena *g_report_arena;

void report_undeclared(Ast_Ident *ident) {
    report_ast_error(ident, "undeclared identifier '%s'.\n", ident->name->data);
}

void report_redeclaration(Ast_Decl *decl) {
    report_ast_error(decl, "redeclaration of '%s'.\n", decl->name->data);
}

internal Report *submit_report(Source_File *file, Report_Kind kind, String8 message, Source_Pos pos, Ast *node) {
    Report *result = push_array(g_report_arena, Report, 1);
    result->kind = kind;
    result->message = message;
    result->source_pos = pos; 
    result->node = node;

    if (kind == REPORT_NOTE) {
        Assert(file->reports.count > 0);
        Report *parent = file->reports.back();
        parent->children.push(result);
    } else {
        file->reports.push(result);
    }


#ifdef _DEBUG
    print_report(result, file);
#endif

    return result;
} 

internal void report_note(Source_Pos pos, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 message = str8_pushfv(g_report_arena, fmt, args);
    va_end(args);

    Source_File *file = pos.file;
    Report *report = submit_report(file, REPORT_NOTE, message, pos);
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

    Report *report = submit_report(file, REPORT_AST_ERROR, message, node->start, node);
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

internal u64 get_next_line_boundary(String8 string, u64 start) {
    for (u64 i = start; i < string.count; i++) {
        if (string.data[i] == '\n' || string.data[i] == '\r') {
            return i;
        }
    }
    return string.count - 1;
} 

#define ANSI_RESET     "\x1B[0m"
#define ANSI_UNDERLINE "\x1B[4m"
#define ANSI_ITALIC    "\x1B[3m"

internal void print_report(Report *report, Source_File *file) {
    switch (report->kind) {
    case REPORT_NOTE:
    {
        printf("%s:%llu:%llu: note: %s", file->path.data, report->source_pos.line, report->source_pos.col, report->message.data);
        break;
    }
    case REPORT_PARSER_ERROR:
    {
        printf("%s:%llu:%llu: syntax error: %s", file->path.data, report->source_pos.line, report->source_pos.col, report->message.data);
        break;
    }
    case REPORT_AST_ERROR:
    {
        printf("%s:%llu:%llu: error: %s", file->path.data, report->node->start.line, report->node->start.col, report->message.data);
        String8 buffer = file->text;
        u64 line_begin = report->node->start.index - report->node->start.col;
        u64 line_end = get_next_line_boundary(buffer, report->node->start.index);

        if (line_begin != report->node->start.col) {
            String8 pre = str8(buffer.data + line_begin, report->node->start.col);
            printf("\x1B[38;2;168;153;132m");
            printf("%.*s", (int)pre.count, pre.data);
            printf(ANSI_RESET);
        }

        u64 end_index = report->node->end.index;
        if (end_index > line_end) end_index = line_end;

        String8 elem_string = str8(buffer.data + report->node->start.index, end_index - report->node->start.index);
        printf("\x1B[38;2;204;36;29m");
        printf("%.*s", (int)elem_string.count, elem_string.data);
        printf(ANSI_RESET);

        if (line_end > end_index) {
            String8 trailing = str8(buffer.data + end_index, line_end - end_index);
            printf("\x1B[38;2;168;153;132m");
            printf("%.*s", (int)trailing.count, trailing.data);
            printf(ANSI_RESET);
        }

        if (report->node->end.index > line_end) {
            printf(ANSI_ITALIC "\x1B[38;2;168;153;132m");
            printf("...");
            printf(ANSI_RESET);
        }
        printf("\n");
        break;
    }
    }
}
