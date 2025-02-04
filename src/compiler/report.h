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
};

#endif // REPORT_H
