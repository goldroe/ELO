#ifndef PARSER_H
#define PARSER_H

struct Parser {
    Lexer *lexer = NULL;
    Ast_Root *root = NULL;

    int error_count = 0;

    bool allow_value_decl = false;
    bool allow_type = false;

    Source_File *file;
    
    Parser(Lexer *_lexer);

    bool load_next_source_file();


    Ast_Paren *Parser::parse_paren_expr();

    Token Parser::expect_token(Token_Kind token);
    void expect_semi();


    Array<Ast_Enum_Field*> Parser::parse_enum_field_list();
    Array<Ast_Value_Decl*> Parser::parse_struct_members();
    Ast_Value_Decl *parse_struct_member();

    Ast_Ident *Parser::parse_ident();

    Ast_Compound_Literal *parse_compound_literal(Ast *operand);
    Ast_Selector *parse_selector_expr(Ast *base);
    Ast_Subscript *parse_subscript_expr(Ast *base);
    Ast_Call *parse_call_expr(Ast *expr);

    Ast *parse_primary_expr(Ast *operand);
    Ast *parse_unary_expr();
    Ast *parse_binary_expr(Ast *lhs, int prec);
    Ast_Cast *parse_cast_expr();
    Ast *parse_assignment_expr();
    Ast *parse_range_expr();
    Ast *parse_expr();

    Ast *parse_operand();

    Array<Ast*> parse_expr_list();

    Ast *parse_import_stmt(Token token);
    Ast *parse_load_stmt(Token token);



    Array<Ast*> Parser::parse_type_list();
    Ast_Block *parse_block();
    Ast_Return *Parser::parse_return_stmt();
    Ast_Continue *parse_continue_stmt();

    Ast_If *parse_if_stmt();

    Ast_Case_Label *Parser::parse_case_clause();
    Ast_Ifcase *parse_ifcase_stmt();
    Ast_While *parse_while_stmt();
    Ast *parse_for_stmt();

    // Ast_Decl_Stmt *parse_init_stmt(Ast *lhs);

    Ast_Value_Decl *parse_value_decl(Array<Ast*> names);

    Ast *parse_simple_stmt();
    Ast *parse_stmt();


    Ast_Proc_Type *Parser::parse_proc_type();

    Ast *parse_type();
    Ast_Param *parse_param();

    void parse_load_or_import();

    void parse();
};

#endif // PARSER_H
