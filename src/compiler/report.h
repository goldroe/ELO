#ifndef REPORT_H
#define REPORT_H

struct Ast;

internal void report_parser_error(Lexer *lexer, const char *fmt, ...);
internal void report_ast_error(Ast *node, const char *fmt, ...);
internal void report_note(Source_Pos pos, const char *fmt, ...);

#endif // REPORT_H
