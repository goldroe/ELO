#ifndef PARSER_H
#define PARSER_H

struct Parser {
    Lexer *lexer = NULL;
    Ast_Root *root = NULL;

    int error_count = 0;
    
    Parser(Lexer *_lexer);

    bool load_next_source_file();

    void expect(Token_Kind token);


    Ast_Ident *Parser::parse_ident();

    Ast_Compound_Literal *parse_compound_literal();
    Ast_Access *parse_access_expr(Ast_Expr *base);
    Ast_Subscript *parse_subscript_expr(Ast_Expr *base);
    Ast_Call *parse_call_expr(Ast_Expr *expr);
    Ast_Expr *parse_unary_expr();
    Ast_Expr *parse_binary_expr(Ast_Expr *lhs, int prec);
    Ast_Expr *parse_postfix_expr();
    Ast_Expr *parse_primary_expr();
    Ast_Cast *parse_cast_expr();
    Ast_Expr *parse_assignment_expr();
    Ast_Expr *parse_range_expr();
    Ast_Expr *parse_expr();

    Auto_Array<Ast_Expr*> parse_expr_list();


    Ast *parse_import_stmt();
    Ast *parse_load_stmt();

    Ast_Block *parse_block();
    Ast_Return *parse_return_stmt();
    Ast_Continue *parse_continue_stmt();

    Ast_If *parse_if_stmt();
    Ast_Ifcase *parse_ifcase_stmt();
    Ast_Case_Label *parse_case_label(Ast_Ifcase *ifcase);
    Ast_While *parse_while_stmt();
    Ast_For *parse_for_stmt();

    Ast_Decl_Stmt *parse_init_stmt(Ast_Expr *lhs);
    Ast *parse_operand(Atom *name);

    Ast *parse_decl_or_value(Ast_Expr *name);
    Ast *parse_simple_stmt();
    Ast *parse_stmt();

    Ast_Type_Defn *parse_type();
    Ast_Param *parse_param();

    Ast_Enum_Field *parse_enum_field();
    Ast_Enum *parse_enum(Atom *name);

    Ast_Struct_Field *parse_struct_field();
    Ast_Struct *parse_struct(Atom *name);
    Ast_Type_Decl *parse_type_decl(Atom *name);
    Ast_Var *parse_var(Atom *name);
    Ast_Operator_Proc *parse_operator_proc();
    Ast_Proc *parse_proc(Atom *name);
    Ast_Decl *parse_decl();

    void parse_load_or_import();

    void parse();
};

#endif // PARSER_H
