#ifndef PARSER_H
#define PARSER_H

struct Parser {
    Lexer *lexer = NULL;
    Ast_Root *root = NULL;

    int error_count = 0;
    
    Parser(Lexer *_lexer);

    void syntax_error(const char *fmt, ...);
    void expect(Token_Kind token);

    Ast_Compound_Literal *parse_compound_literal();
    Ast_Expr *parse_expr();
    Ast_Expr *parse_range_expr();
    Ast_Expr *parse_assignment_expr();
    Ast_Expr *parse_unary_expr();
    Ast_Expr *parse_binary_expr(Ast_Expr *lhs, int prec);
    Ast_Expr *parse_postfix_expr();
    Ast_Expr *parse_primary_expr();

    Ast_If *parse_if_stmt();
    Ast_While *parse_while_stmt();
    Ast_For *parse_for_stmt();
    Ast_Decl_Stmt *parse_init_stmt(Ast_Expr *lhs);
    Ast_Stmt *parse_simple_stmt();
    Ast_Stmt *parse_stmt();
    Ast_Block *parse_block();

    Ast_Type_Defn *parse_type();
    Ast_Param *parse_param();

    Ast_Enum_Field *parse_enum_field();
    Ast_Enum *parse_enum(Token name);

    Ast_Struct_Field *parse_struct_field();
    Ast_Struct *parse_struct(Token name);

    Ast_Var *parse_var(Atom *name);
    Ast_Proc *parse_proc(Token name);
    Ast_Decl *parse_decl();

    void parse();
};

#endif // PARSER_H
