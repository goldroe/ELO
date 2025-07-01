
Parser::Parser(Lexer *_lexer) {
    this->lexer = _lexer;
    _lexer->parser = this;

    add_source_file(lexer->source_file);
}

bool Parser::load_next_source_file() {
    Source_File *source_file = lexer->source_file->next;
    if (source_file) {
        lexer->set_source_file(source_file);
        file = source_file;
        return true;
    } else {
        return false;
    }
}

Token Parser::expect_token(Token_Kind kind) {
    Token token = lexer->current();
    if (lexer->match(kind)) {
        lexer->next_token();
    } else {
        report_parser_error(lexer, "expected '%s', got '%s'.\n", string_from_token(kind), string_from_token(token.kind));
    }
    return token;
}

Auto_Array<Ast*> Parser::parse_expr_list() {
    Auto_Array<Ast*> list;

    Ast *expr = parse_expr();
    if (!expr) {
        return list;
    }
    list.push(expr);
    while (lexer->eat(TOKEN_COMMA)) {
        expr = parse_expr();
        if (!expr) {
            report_parser_error(lexer, "expected expression after ','.\n");
            break;
        }
        list.push(expr);
    }
    
    return list;
}

Auto_Array<Ast*> Parser::parse_type_list() {
    Auto_Array<Ast*> type_list = {};

    for (;;) {
        Ast *type = parse_type();
        if (!type) {
            break;
        }

        type_list.push(type);

        if (!lexer->eat(TOKEN_COMMA)) {
            break; 
        }
    }

    return type_list;
}

Ast_Compound_Literal *Parser::parse_compound_literal(Ast *operand) {
    Token open = expect_token(TOKEN_LBRACE);

    Auto_Array<Ast*> elements = {};

    while (!lexer->match(TOKEN_RBRACE)) {
        Ast *expr = parse_expr(); 
        if (expr == NULL) break;

        elements.push(expr);

        if (!lexer->eat(TOKEN_COMMA)) break;
    }

    Token close = expect_token(TOKEN_RBRACE);

    Ast_Compound_Literal *compound = ast_compound_literal(file, open, close, operand, elements);
    return compound;
}

Ast_Selector *Parser::parse_selector_expr(Ast *base) {
    Token token = expect_token(TOKEN_DOT);

    Token name = lexer->current();
    if (!lexer->eat(TOKEN_IDENT)) {
        report_parser_error(lexer, "missing name after '.'\n");
    }

    return ast_selector_expr(file, token, base, ast_ident(file, name));
}

Ast_Subscript *Parser::parse_subscript_expr(Ast *base) {
    Token open = expect_token(TOKEN_LBRACKET);
    
    Ast *index = parse_expr();
    if (index == NULL) {
        report_parser_error(lexer, "missing subscript expression.\n");
    }

    Token close = expect_token(TOKEN_RBRACKET);

    return ast_subscript_expr(file, open, close, base, index);
}

Ast_Call *Parser::parse_call_expr(Ast *expr) {
    Token open = expect_token(TOKEN_LPAREN);

    Auto_Array<Ast*> arguments = {};
    do {
        if (lexer->match(TOKEN_RPAREN)) {
            break;   
        }
        Ast *arg = parse_expr();
        if (!arg) break;
        arguments.push(arg);
    } while (lexer->eat(TOKEN_COMMA));

    Token close = expect_token(TOKEN_RPAREN);

    return ast_call_expr(file, open, close, expr, arguments);
}

Ast_Cast *Parser::parse_cast_expr() {
    Token token = expect_token(TOKEN_CAST);

    expect_token(TOKEN_LPAREN);
    Ast *type = parse_type();
    expect_token(TOKEN_RPAREN);

    Ast *elem = parse_unary_expr();

    if (!type) {
        report_parser_error(lexer, "missing type in cast expression.\n");
    }

    if (!elem) {
        report_parser_error(lexer, "missing expression after cast.\n");
    }

    return ast_cast_expr(file, token, type, elem);
}

Ast *Parser::parse_range_expr() {
    Ast *lhs = parse_expr();

    if (lexer->match(TOKEN_ELLIPSIS)) {
        Token token = expect_token(TOKEN_ELLIPSIS);
        Ast *rhs = parse_expr();
        if (!rhs) {
            report_parser_error(lexer, "missing expression after '..'.\n");
        }
        return ast_range_expr(file, token, lhs, rhs);
    } else {
        return lhs;
    }
}

Ast_Value_Decl *Parser::parse_struct_member() {
    Auto_Array<Ast*> lhs = {};

    Token token = lexer->current();

    //@Note Anonymous members
    if (token.kind == TOKEN_STRUCT || token.kind == TOKEN_ENUM || token.kind == TOKEN_UNION) {
        Ast *operand = parse_operand();

        Auto_Array<Ast*> values = {operand};

        return ast_value_decl(file, lhs, nullptr, values, false);
    }
    
    Ast *type = nullptr;
    Auto_Array<Ast*> values = {};
    lhs = parse_expr_list();

    expect_token(TOKEN_COLON);

    //@Note Multiple values require type, '::' constants not allowed
    if (lhs.count > 1) {
        type = parse_type();

        if (!type) {
            report_parser_error(lexer, "expected a type.\n");
        }

        expect_semi();

        return ast_value_decl(file, lhs, type, values, true);
    }

    return parse_value_decl(lhs);
}

Auto_Array<Ast_Value_Decl*> Parser::parse_struct_members() {
    Auto_Array<Ast_Value_Decl*> members = {};
    while (!lexer->match(TOKEN_RBRACE)) {
        Ast_Value_Decl *member = parse_struct_member();
        if (!member) break;
        members.push(member);
    }
    return members;
}

Auto_Array<Ast_Enum_Field*> Parser::parse_enum_field_list() {
    Auto_Array<Ast_Enum_Field*> field_list;

    while (!lexer->match(TOKEN_RBRACE)) {
        Ast_Ident *ident = parse_ident();

        Ast *expr = nullptr;
        if (lexer->eat(TOKEN_EQ)) {
            expr = parse_expr();
        }

        Ast_Enum_Field *enum_field = ast_enum_field(file, ident, expr);
        field_list.push(enum_field);

        if (!lexer->eat(TOKEN_COMMA)) {
            break; 
        }
    }

    return field_list;
}

// Ast_Type_Decl *Parser::parse_type_decl(Atom *name) {
//     expect_token(TOKEN_TYPEDEF);

//     Ast_Type_Decl *type_decl = ast_type_decl(name, nullptr);

//     Ast *type = parse_type();
//     type_decl->type_defn = type_defn;

//     if (!type) {
//         report_parser_error(lexer, "missing type after #type.\n");
//         type_decl->poison();
//         return type_decl;
//     }

//     type_decl->mark_end(type->end);

//     return type_decl;
// }

// Ast_Var *Parser::parse_var(Atom *name) {
//     Ast *init = NULL;
//     Ast *type = NULL;

//     if (lexer->eat(TOKEN_COLON)) {
//         type = parse_type();
//         if (!type) {
//             report_parser_error(lexer, "expected type after ':'.\n");
//             goto ERROR_BLOCK;
//         }
//         if (lexer->eat(TOKEN_EQ)) {
//             init = parse_expr();
//             if (!init) {
//                 report_parser_error(lexer, "expected expression after '='.\n");
//                 goto ERROR_BLOCK;
//             }
//         }
//     } else if (lexer->eat(TOKEN_COLON_EQ)) {
//         init = parse_expr();
//         if (!init) {
//             report_parser_error(lexer, "expected expression after ':='.\n");
//             goto ERROR_BLOCK;
//         }
//     } else {
//         Assert(0);
//     }

//     Ast_Var *var = ast_var(name, init, type);
//     if (var->init) {
//         var->mark_end(init->end);
//     } else {
//         var->mark_end(type_defn->end);
//     }
//     return var;

// ERROR_BLOCK:
//     Ast_Var *err = ast_var(name, init, type);
//     err->poison();
//     return err;
// }

// Ast_Proc *Parser::parse_proc(Token name) {
//     bool has_varargs = false;
//     Auto_Array<Ast_Param*> parameters;
//     expect_token(TOKEN_LPAREN);
//     while (!lexer->match(TOKEN_RPAREN)) {
//         Ast_Param *param = parse_param();
//         if (param == NULL) break;
//         parameters.push(param);

//         if (has_varargs && param->is_vararg) {
//             report_parser_error(lexer, "Variadic parameter must be used only once.\n");
//         } else if (has_varargs) {
//             report_parser_error(lexer, "Variadic parameter must be last.\n");
//         }

//         if (param->is_vararg) has_varargs = true;

//         if (!lexer->eat(TOKEN_COMMA)) {
//             break;
//         }
//     }
//     expect_token(TOKEN_RPAREN);

//     Ast *return_type = NULL;
//     if (lexer->eat(TOKEN_ARROW)) {
//         return_type = parse_type();
//     }

//     bool is_foreign = false;
//     Ast_Block *block = NULL;
//     if (lexer->match(TOKEN_LBRACE)) {
//         block = parse_block();
//     } else if (lexer->eat(TOKEN_FOREIGN)) {
//         is_foreign = true;
//     }
//     Source_Pos end = lexer->current().start;
    
//     Ast_Proc *proc = ast_proc(name, parameters, return_type, block);
//     proc->foreign = is_foreign;
//     proc->has_varargs = has_varargs;
//     proc->mark_end(end);
//     return proc;
// }

// Ast_Operator_Proc *Parser::parse_operator_proc() {
//     Source_Pos start = lexer->current().start;
    
//     expect_token(TOKEN_OPERATOR);

//     Ast_Operator_Proc *proc = NULL;

//     Token op_tok = lexer->current();

//     Auto_Array<Ast_Param*> parameters;

//     if (is_operator(op_tok.kind)) {
//         OP op = {}; //@todo get operator
//         lexer->next_token();

//         if (op_tok.kind == TOKEN_LBRACKET) {
//             expect_token(TOKEN_RBRACKET);
//         }

//         if (!lexer->eat(TOKEN_COLON2)) {
//             report_parser_error(lexer, "missing '::', got '%s'.\n", string_from_token(lexer->peek()));
//         }

//         if (operator_is_overloadable(op_tok.kind)) {
//             if (!lexer->eat(TOKEN_LPAREN)) {
//                 report_parser_error(lexer, "missing '('.\n");
//                 goto ERROR_HANDLE;
//             }

//             while (!lexer->match(TOKEN_RPAREN)) {
//                 Ast_Param *param = parse_param();
//                 if (param == NULL) break;
//                 parameters.push(param);
//                 if (!lexer->eat(TOKEN_COMMA)) {
//                     break;
//                 }
//             }

//             Source_Pos end = lexer->current().end;

//             if (!lexer->eat(TOKEN_RPAREN)) {
//                 report_parser_error(lexer, "missing ')'.\n");
//                 goto ERROR_HANDLE;
//             }

//             Ast *return_type = NULL;
//             if (lexer->eat(TOKEN_ARROW)) {
//                 return_type = parse_type();
//             }

//             Ast_Block *block = parse_block();
//             proc = ast_operator_proc(op, parameters, return_type, block);
//             proc->mark_range(start, end);
//         } else {
//             report_parser_error(lexer, "invalid operator, cannot overload '%s'.\n", string_from_token(op_tok.kind));
//             goto ERROR_HANDLE;
//         }
//     } else {
//         report_parser_error(lexer, "expected operator, got '%s'.\n", string_from_token(op_tok.kind));
//         goto ERROR_HANDLE;
//     } 

//     return proc;

// ERROR_HANDLE:
//     return NULL;
// }

Ast_Paren *Parser::parse_paren_expr() {
    Token open = expect_token(TOKEN_LPAREN);
    Ast *elem = parse_expr();
    if (!elem) {
        report_parser_error(lexer, "missing expression in paren.\n");
    }
    Token close = expect_token(TOKEN_RPAREN);
    return ast_paren_expr(file, open, close, elem);
}

Ast *Parser::parse_primary_expr(Ast *operand) {
    if (!operand) return operand;

    bool loop = true;
    while (loop) {
        Token token = lexer->current();

        switch (token.kind) {
        default:
            loop = false;
            break;

        //@Fix Breaks control structure expressions
        // case TOKEN_LBRACE: {
        //     Ast_Compound_Literal *compound = parse_compound_literal(operand);
        //     return compound;
        // }

        case TOKEN_DOT:
            operand = parse_selector_expr(operand);
            break;

        case TOKEN_DOT_STAR:
            expect_token(TOKEN_DOT_STAR);
            operand = ast_deref_expr(file, token, operand);
            break;

        case TOKEN_LBRACKET:
            operand = parse_subscript_expr(operand);
            break;

        case TOKEN_LPAREN:
            operand = parse_call_expr(operand);
            break;
        }
    }

    return operand;
}

Ast *Parser::parse_operand() {
    Ast *operand = nullptr;

    Token token = lexer->current();
    switch (token.kind) {
    case TOKEN_IDENT:
        expect_token(TOKEN_IDENT);
        return ast_ident(file, token);

    case TOKEN_INTEGER:
    case TOKEN_FLOAT:
    case TOKEN_STRING:
        lexer->next_token();
        operand = ast_literal(file, token);
        return operand;

    case TOKEN_STAR: {
        expect_token(TOKEN_STAR);
        Ast *type = parse_type();
        operand = ast_pointer_type(file, token, type);
        return operand;
    }
    case TOKEN_LBRACKET: {
        expect_token(TOKEN_LBRACKET);
        Ast *length = parse_expr();
        expect_token(TOKEN_RBRACKET);
        Ast *type = parse_type();
        operand = ast_array_type(file, token, type, length);
        return operand;
    }

    case TOKEN_LPAREN: {
        if (!allow_value_decl) {
            Ast_Paren *paren = parse_paren_expr();
        }

        Ast_Proc_Type *type = parse_proc_type();

        bool is_foreign = false;
        if (lexer->eat(TOKEN_FOREIGN)) {
            is_foreign = true;
        }

        type->foreign = is_foreign;

        if (allow_type) {
            return type;
        }

        Ast_Block *body = nullptr;
        if (lexer->match(TOKEN_LBRACE)) {
            body = parse_block();
        }

        if (is_foreign && body) {
            report_parser_error(lexer, "body not set for foreign procedure.\n");
        } else if (!is_foreign && !body) {
            report_parser_error(lexer, "missing procedure body.\n");
        }
        return ast_proc_lit(file, type, body);
    }

    case TOKEN_UNION:
    case TOKEN_STRUCT: {
        lexer->next_token();

        Token open = lexer->current();
        Token close = lexer->current();

        Auto_Array<Ast_Value_Decl*> members = {};
        if (lexer->match(TOKEN_LBRACE)) {
            open = expect_token(TOKEN_LBRACE);
            members = parse_struct_members();
            close = expect_token(TOKEN_RBRACE);
        }

        if (token.kind == TOKEN_STRUCT) return ast_struct_type(file, token, open, close, members);

        if (token.kind == TOKEN_UNION) return ast_union_type(file, token, open, close, members);
    }

    case TOKEN_ENUM: {
        expect_token(TOKEN_ENUM);

        Ast *base_type = nullptr;
        if (!lexer->match(TOKEN_LBRACE)) {
            base_type = parse_type();
        }

        Token open = expect_token(TOKEN_LBRACE);

        Auto_Array<Ast_Enum_Field*> field_list = parse_enum_field_list();

        Token close = expect_token(TOKEN_RBRACE);

        return ast_enum_type(file, token, open, close, base_type, field_list);
    }

    case TOKEN_TYPEDEF: {
        // return parse_type_decl(name);
    }

    case TOKEN_SIZEOF: {
        expect_token(TOKEN_SIZEOF);

        Token open = expect_token(TOKEN_LPAREN);

        Ast *elem = parse_expr();

        Token close = expect_token(TOKEN_RPAREN);

        Ast_Sizeof *size_of = ast_sizeof_expr(file, token, open, close, elem);
        return size_of;
    }
    }

    return operand;
}

Ast *Parser::parse_unary_expr() {
    Token token = lexer->current();
    switch (token.kind) {
    case TOKEN_CAST: {
        Ast_Cast *expr = parse_cast_expr();
        return expr;
    }

    case TOKEN_STAR: {
        expect_token(TOKEN_STAR);
        Ast *elem = parse_unary_expr();
        Ast_Address *expr = ast_address_expr(file, token, elem);
        return expr;
    }

    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_BANG:
    case TOKEN_SQUIGGLE: {
        OP op = get_unary_operator(token.kind);
        Ast *elem = parse_unary_expr();
        Ast_Unary *expr = ast_unary_expr(file, token, op, elem);
        return expr;
    }
    }

    return parse_primary_expr(parse_operand());
}

Ast *Parser::parse_binary_expr(Ast *lhs, int current_prec) {
    for (;;) {
        Token token = lexer->current();
        OP op = get_binary_operator(token.kind);
        int prec = get_operator_precedence(op);

        if (prec < current_prec) {
            return lhs;
        }

        lexer->next_token();

        Ast *rhs = parse_unary_expr();
        if (rhs == nullptr) {
            report_parser_error(lexer, "expected expression after '%s'.\n", string_from_operator(op));
            lhs->poison();
            return lhs;
        }

        Token next_token = lexer->current();
        OP next_op = get_binary_operator(next_token.kind);
        int next_prec = get_operator_precedence(next_op);

        if (prec < next_prec) {
            rhs = parse_binary_expr(rhs, prec + 1);

            //@Note Missing expression on the right side of the operator
            if (rhs == nullptr) {
                rhs = ast_binary_expr(file, token, op, lhs, nullptr);
                rhs->poison();
                return rhs;
            }
        }

        lhs = ast_binary_expr(file, token, op, lhs, rhs);
    }
}

Ast *Parser::parse_expr() {
    Ast *expr = parse_unary_expr();
    if (expr) {
        expr = parse_binary_expr(expr, 0);
    }
    return expr;
}

Ast_Block *Parser::parse_block() {
    Token open = expect_token(TOKEN_LBRACE);

    Auto_Array<Ast*> statements;
    while (!lexer->match(TOKEN_RBRACE)) {
        Ast *stmt = parse_stmt();
        if (stmt == NULL) break;
        statements.push(stmt);
    }
    Token close = expect_token(TOKEN_RBRACE);

    Ast_Block *block = ast_block_stmt(file, open, close, statements);

    return block;
}

Ast_If *Parser::parse_if_stmt() {
    Token token = lexer->current();
    expect_token(TOKEN_IF);

    Ast *cond = parse_expr();
    Ast_Block *block = parse_block();
    Ast_If *if_stmt = ast_if_stmt(file, token, cond, block);

    if (!cond) {
        report_parser_error(lexer, "missing condition for if statement.\n");
        if_stmt->poison();
    }

    Ast_If *else_stmt = nullptr;

    if (lexer->match(TOKEN_ELSE)) {
        token = expect_token(TOKEN_ELSE);
        switch (lexer->peek()) {
        case TOKEN_IF: {
            else_stmt = parse_if_stmt();
            break;
        }
        case TOKEN_LBRACE: {
            Ast_Block *block = parse_block();
            else_stmt = ast_if_stmt(file, token, nullptr, block);
            else_stmt->is_else = true;
            break;
        }
        }
    }

    if_stmt->next = else_stmt;
    if (else_stmt) {
        else_stmt->prev = if_stmt;
    }

    return if_stmt;
}

Ast_While *Parser::parse_while_stmt() {
    Token token = expect_token(TOKEN_WHILE);

    Ast *cond = parse_expr();
    if (!cond) {
        report_parser_error(lexer, "missing while loop expression.\n");
    }

    Ast_Block *block = parse_block();
    return ast_while_stmt(file, token, cond, block);
}

Ast *Parser::parse_for_stmt() {
    Token token = expect_token(TOKEN_FOR);

    //@Note for {..}
    if (lexer->match(TOKEN_LBRACE)) {
        Ast_Block *block = parse_block();
        return ast_for_stmt(file, token, {}, nullptr, block);
    }

    Auto_Array<Ast*> lhs = parse_expr_list();

    if (lhs.count == 0) {
        report_parser_error(lexer, "expected expression, got '%s'.\n", string_from_token(lexer->peek()));
        return ast_bad_stmt(file, lexer->current(), lexer->current());
    }

    expect_token(TOKEN_COLON);

    Ast *init = parse_range_expr();

    if (!init) {
        report_parser_error(lexer, "expected expression, got '%s'.\n", string_from_token(lexer->peek()));
        return ast_bad_stmt(file, lexer->current(), lexer->current());
    }

    Ast_Block *block = parse_block();

    return ast_for_stmt(file, token, lhs, init, block);
}

Ast_Case_Label *Parser::parse_case_clause() {
    Token token = expect_token(TOKEN_CASE);

    Ast *cond = parse_range_expr();

    expect_token(TOKEN_COLON);

    Auto_Array<Ast*> statements;

    for (;;) {
        Ast *stmt = parse_stmt();
        if (!stmt) break;
        statements.push(stmt);
    }

    for (Ast *stmt : statements) {
        if (stmt->kind == AST_FALLTHROUGH) {
            if (stmt != statements.back()) {
                report_ast_error(stmt, "illegal fallthrough, must be placed at end of a case block.\n");
            }
        }
    }

    return ast_case_label(file, token, cond, statements);
}

Ast_Ifcase *Parser::parse_ifcase_stmt() {
    Token token = lexer->current();
    expect_token(TOKEN_IFCASE);

    Ast *cond = parse_expr();

    bool check_complete = false;

    if (lexer->eat(TOKEN_COMPLETE)) {
        check_complete = true;
    }

    Token open = expect_token(TOKEN_LBRACE);

    Auto_Array<Ast_Case_Label*> clauses = {};

    Ast_Case_Label *prev_clause = nullptr;

    while (!lexer->match(TOKEN_RBRACE)) {
        if (!lexer->match(TOKEN_CASE)) {
            break;
        }

        Ast_Case_Label *case_clause = parse_case_clause();

        case_clause->prev = prev_clause;
        if (prev_clause) {
            prev_clause->next = case_clause;
        }
        prev_clause = case_clause;

        clauses.push(case_clause);
    }

    Token close = expect_token(TOKEN_RBRACE);

    return ast_ifcase_stmt(file, token, open, close, cond, clauses, check_complete);
}

Ast *Parser::parse_load_stmt() {
    Token token = expect_token(TOKEN_LOAD);

    if (!lexer->match(TOKEN_STRING)) {
        report_parser_error(lexer, "missing filename after '#load'.\n");
        return ast_bad_stmt(file, token, token);
    }

    Token file_token = expect_token(TOKEN_STRING);
    String file_path = file_token.value.value_string;

    if (path_is_relative(file_path)) {
        String current_dir = path_dir_name(lexer->source_file->path);
        file_path = path_join(heap_allocator(), current_dir, file_path);
    }
    Source_File *source_file = source_file_create(file_path);

    if (!file) {
        report_line("path does not exit: %s", file_path.data);
        return ast_bad_stmt(file, file_token, file_token);
    }

    expect_semi();

    add_source_file(source_file);

    return ast_load_stmt(file, token, file_token, file_path);
}

Ast *Parser::parse_import_stmt() {
    Token token = expect_token(TOKEN_IMPORT);
    if (!lexer->match(TOKEN_STRING)) {
        report_parser_error(lexer, "missing filename after '#import'.\n");
        return ast_bad_stmt(file, token, token);
    }

    Token file_token = expect_token(TOKEN_STRING);
    String file_path = file_token.value.value_string;

    String import_path = os_exe_path(heap_allocator());
    import_path = normalize_path(heap_allocator(), import_path);
    import_path = path_join(heap_allocator(), import_path, str_lit("core"));
    import_path = path_join(heap_allocator(), import_path, file_path);
    Source_File *source_file = source_file_create(import_path);

    if (!file) {
        report_line("path does not exit: %s", file_path.data);
        return ast_bad_stmt(file, file_token, file_token);
    }

    expect_semi();

    add_source_file(source_file);

    return ast_import_stmt(file, token, file_token, file_path);
}

Ast_Proc_Type *Parser::parse_proc_type() {
    Auto_Array<Ast_Param*> params = {};

    bool has_varargs = false;

    Token open = expect_token(TOKEN_LPAREN);

    while (!lexer->match(TOKEN_RPAREN)) {
        if (lexer->match(TOKEN_ELLIPSIS)) {
            Token token = expect_token(TOKEN_ELLIPSIS);
            Ast_Ident *ident = ast_ident(file, token);
            ident->name = atom_create(str_lit(".."));
            Ast_Param *param = ast_param(file, ident, nullptr);
            param->is_vararg = true;
            params.push(param);
            continue;
        }

        Ast_Ident *ident = parse_ident();

        expect_token(TOKEN_COLON);

        Ast *type = parse_type();

        Ast_Param *param = ast_param(file, ident, type);

        params.push(param);

        if (!lexer->eat(TOKEN_COMMA)) {
            break;
        }
    }

    Token close = expect_token(TOKEN_RPAREN);

    Auto_Array<Ast*> return_types = {};
    if (lexer->eat(TOKEN_ARROW)) {
        return_types = parse_type_list();
    }

    return ast_proc_type(file, open, close, params, return_types);
}

Ast *Parser::parse_type() {
    bool prev_allow_type = allow_type;
    allow_type = true;

    Ast *type = parse_operand();
    while (lexer->match(TOKEN_DOT)) {
        Ast_Selector *selector = parse_selector_expr(type);
        type = selector;
    }

    allow_type = prev_allow_type;
    return type;
}

Ast_Value_Decl *Parser::parse_value_decl(Auto_Array<Ast*> names) {
    allow_value_decl = true;

    Ast *type = parse_type();

    allow_value_decl = false;

    Token token = lexer->current();

    for (Ast *name : names) {
        if (name->kind != AST_IDENT) {
            report_ast_error(name, "cannot assign to lhs.\n");
        }
    }

    bool is_mutable = true;

    Auto_Array<Ast*> values = {};
    
    if (token.kind == TOKEN_COLON || token.kind == TOKEN_EQ) {
        lexer->next_token();
        is_mutable = token.kind != TOKEN_COLON;

        allow_value_decl = true;

        values = parse_expr_list();

        allow_value_decl = false;
    }

    bool semi = true;
    if (values.count != 0) {
        Ast *value = values.front();
        if (value->kind == AST_PROC_LIT ||
            value->kind == AST_STRUCT_TYPE || 
            value->kind == AST_ENUM_TYPE) semi = false;
    }

    if (semi) {
        expect_semi();
    }

    return ast_value_decl(file, names, type, values, is_mutable);
}

Ast *Parser::parse_simple_stmt() {
    Auto_Array<Ast*> lhs = parse_expr_list();

    if (lhs.count == 0) return nullptr;

    Token token = lexer->current();
    switch (lexer->peek()) {
    case TOKEN_EQ:
    case TOKEN_PLUS_EQ:
    case TOKEN_MINUS_EQ:
    case TOKEN_STAR_EQ:
    case TOKEN_SLASH_EQ:
    case TOKEN_MOD_EQ:
    case TOKEN_XOR_EQ:
    case TOKEN_BAR_EQ:
    case TOKEN_AMPER_EQ:
    case TOKEN_LSHIFT_EQ:
    case TOKEN_RSHIFT_EQ: {
        lexer->next_token();
        OP op = get_binary_operator(token.kind);
        Auto_Array<Ast*> rhs = parse_expr_list();
        if (rhs.count == 0) {
            report_parser_error(lexer, "missing rhs in assignment statement.\n");
            return ast_bad_stmt(file, token, lexer->current());
        }
        expect_semi();
        Ast_Assignment *assignment = ast_assignment_stmt(file, token, op, lhs, rhs);
        return assignment;
    }

    case TOKEN_COLON: {
        expect_token(TOKEN_COLON);
        Ast_Value_Decl *value_decl = parse_value_decl(lhs);
        return value_decl;
    }
    }

    if (lhs.count > 1) {
        report_parser_error(lexer, "expected just one expression.\n");
    }

    Ast_Stmt *expr_stmt = ast_expr_stmt(file, lhs.front());
    return expr_stmt;
}

void Parser::expect_semi() {
    expect_token(TOKEN_SEMI);
}

Ast_Return *Parser::parse_return_stmt() {
    Token token = expect_token(TOKEN_RETURN);

    Auto_Array<Ast*> values = parse_expr_list();

    expect_semi();

    return ast_return_stmt(file, token, values);
}

Ast *Parser::parse_stmt() {
    Ast *stmt = nullptr;

    Token token = lexer->current();
    switch (token.kind) {
    case TOKEN_IF:
        stmt = parse_if_stmt();
        return stmt;
    case TOKEN_ELSE:
        report_parser_error(lexer, "illegal else without matching if.\n");
        stmt = ast_bad_stmt(file, token, token);
        return stmt;

    case TOKEN_IFCASE:
        stmt = parse_ifcase_stmt();
        return stmt;

    case TOKEN_DO:
        return nullptr;

    case TOKEN_WHILE:
        stmt = parse_while_stmt();
        return stmt;

    case TOKEN_FOR:
        stmt = parse_for_stmt();
        return stmt;

    case TOKEN_RETURN:
        stmt = parse_return_stmt();
        return stmt;

    case TOKEN_CONTINUE:
        token = expect_token(TOKEN_CONTINUE);
        expect_semi();
        stmt = ast_continue_stmt(file, token);
        return stmt;

    case TOKEN_BREAK:
        token = expect_token(TOKEN_BREAK);
        expect_semi();
        stmt = ast_break_stmt(file, token);
        return stmt;

    case TOKEN_FALLTHROUGH:
        token = expect_token(TOKEN_FALLTHROUGH);
        expect_semi();
        stmt = ast_fallthrough_stmt(file, token);
        return stmt;

    case TOKEN_DEFER:
        token = expect_token(TOKEN_DEFER);
        stmt = ast_defer_stmt(file, token, parse_stmt());
        return stmt;

    case TOKEN_LBRACE:
        stmt = parse_block();
        return stmt;

    case TOKEN_SEMI:
        expect_semi();
        stmt = ast_empty_stmt(file, token);
        return stmt;

    case TOKEN_IMPORT:
        stmt = parse_import_stmt();
        return stmt;

    case TOKEN_LOAD:
        stmt = parse_load_stmt();
        return stmt;
    }

    stmt = parse_simple_stmt();
    if (stmt && stmt->kind == AST_EXPR_STMT) {
        expect_semi();
    }
    return stmt;
}

Ast_Ident *Parser::parse_ident() {
    Token token = lexer->current();
    if (!lexer->match(TOKEN_IDENT)) {
        token.name = atom_create(str_lit("_"));
    } else {
        expect_token(TOKEN_IDENT);
    }
    return ast_ident(file, token);
}

void Parser::parse() {
    Auto_Array<Ast*> decls = {};

    for (;;) {
        if (lexer->eof()) {
            if (!load_next_source_file()) {
                break;
            }
        }

        Ast *stmt = parse_stmt();
        if (stmt && stmt->kind != AST_EMPTY_STMT) {
            decls.push(stmt);
        }
    }

    root = ast_root(file, decls);
}
