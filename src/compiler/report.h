#ifndef REPORT_H
#define REPORT_H

#include "base/base_memory.h"

#include "ast.h"

struct Ast;

extern Arena *g_report_arena;
extern int g_error_count;


u64 get_line_start_after_spaces(String string, u64 start);

u64 get_next_line_boundary(String string, u64 start);

void report_out_va(const char *fmt, va_list args);

void report_out(const char *fmt, ...);

void report_line(const char *fmt, ...);

void report_error_line(Source_File *file, Source_Pos pos);

void report_parser_error_va(Source_Pos pos, const char *fmt, va_list va);

void report_parser_error(Token token, const char *fmt, ...);

void report_parser_error(Lexer *lexer, const char *fmt, ...);

void report_ast_error_va(Ast *node, const char *fmt, va_list va);

void report_ast_error(Ast *node, const char *fmt, ...);

void report_note_va(Source_Pos pos, const char *fmt, va_list va);

void report_note(Source_Pos pos, const char *fmt, ...);

void report_undeclared(Ast_Ident *ident);

void report_redeclaration(Ast_Decl *decl);

Token ast_start_token(Ast *node);

Token ast_end_token(Ast *node);

#endif // REPORT_H
