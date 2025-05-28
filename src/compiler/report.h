#ifndef REPORT_H
#define REPORT_H

struct Ast;

enum Report_Kind {
    REPORT_NIL,
    REPORT_NOTE,
    REPORT_WARNING,
    REPORT_PARSER_ERROR,
    REPORT_AST_ERROR,
};

struct Report {
    Report_Kind kind;
    String8 message;
    Source_Pos source_pos;
    Ast *node;
    Auto_Array<Report*> children;
};

internal void print_report(Report *report, Source_File *file);
internal Report *submit_report(Source_File *file, Source_File *owner, Report_Kind kind, String8 message, Source_Pos pos, Ast *node = nullptr);
internal void report_parser_error(Lexer *lexer, const char *fmt, ...);
internal void report_ast_error(Ast *node, const char *fmt, ...);

#endif // REPORT_H
