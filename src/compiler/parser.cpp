
Parser::Parser(Lexer *_lexer) {
    this->lexer = _lexer;
    _lexer->parser = this;

    add_source_file(lexer->source_file);
}

bool Parser::load_next_source_file() {
    Source_File *source_file = lexer->source_file->next;
    if (source_file) {
        lexer->set_source_file(source_file);
        return true;
    } else {
        return false;
    }
}

void Parser::expect(Token_Kind token) {
    if (lexer->match(token)) {
        lexer->next_token();
    } else {
        report_parser_error(lexer, "expected '%s', got '%s'.\n", string_from_token(token), string_from_token(lexer->peek()));
    }
}

Ast_Compound_Literal *Parser::parse_compound_literal() {
    Source_Pos start = lexer->current().start;

    Ast_Type_Defn *type_defn = parse_type();;

    expect(TOKEN_LBRACE);

    Auto_Array<Ast_Expr*> elements;

    while (!lexer->eof() && !lexer->match(TOKEN_RBRACE)) {
        Ast_Expr *expr = parse_expr(); 
        if (expr == NULL) break;

        elements.push(expr);

        if (!lexer->eat(TOKEN_COMMA)) break;
    }

    Source_Pos end = lexer->current().end;

    expect(TOKEN_RBRACE);

    Ast_Compound_Literal *compound = ast_compound_literal(elements, type_defn);
    compound->mark_range(start, end);
    return compound;
}

Ast_Expr *Parser::parse_primary_expr() {
    Ast_Expr *expr = NULL;

    Token token = lexer->current();
    switch(token.kind) {

    case TOKEN_DOT:
    {
        lexer->next_token();
        Ast_Compound_Literal *compound = parse_compound_literal();
        compound->mark_start(token.start);
        expr = compound;
        break;
    }

    case TOKEN_LPAREN:
    {
        lexer->next_token();
        Ast_Expr *elem = parse_expr();
        Source_Pos end = lexer->current().end;
        expect(TOKEN_RPAREN);
        Ast_Paren *paren = ast_paren(elem);
        paren->mark_range(token.start, end);
        expr = paren;
        break;
    }

    case TOKEN_NULL:
    {
        lexer->next_token();
        Ast_Null *null = AST_NEW(Ast_Null);
        null->mark_range(token.start, token.end);
        expr = null;
        break;
    }

    case TOKEN_TRUE:
    case TOKEN_FALSE:
    {
        lexer->next_token();
        Ast_Literal *literal = AST_NEW(Ast_Literal);
        literal->literal_flags = LITERAL_BOOLEAN;
        literal->int_val = (token.kind == TOKEN_TRUE);
        literal->mark_range(token.start, token.end);
        expr = literal;
        break;
    }

    case TOKEN_IDENT:
    {
        lexer->next_token();
        Ast_Ident *ident = ast_ident(token);
        expr = ident;
        break;
    }

    case TOKEN_INTLIT:
    {
        lexer->next_token();
        Ast_Literal *literal = ast_intlit(token);
        literal->mark_range(token.start, token.end);
        expr = literal;
        break;
    }

    case TOKEN_FLOATLIT:
    {
        lexer->next_token();
        Ast_Literal *literal = ast_floatlit(token);
        literal->mark_range(token.start, token.end);
        expr = literal;
        break;
    }

    case TOKEN_STRLIT:
    {
        lexer->next_token();
        Ast_Literal *literal = ast_strlit(token);
        literal->mark_range(token.start, token.end);
        expr = literal;
        break;
    }
    }
    return expr;
}

Ast_Access *Parser::parse_access_expr(Ast_Expr *base) {
    Token op = lexer->current();
    expect(TOKEN_DOT);

    Token name = lexer->current();
    if (!lexer->eat(TOKEN_IDENT)) {
        report_parser_error(lexer, "missing name after '.'\n");
    }

    Ast_Access *access = ast_access_expr(base, ast_ident(name));
    access->mark_range(op.start, name.end);
    return access;
}

Ast_Subscript *Parser::parse_subscript_expr(Ast_Expr *base) {
    Token op = lexer->current();
    expect(TOKEN_LBRACKET);
    
    Ast_Expr *index = parse_expr();
    if (index == NULL) {
        report_parser_error(lexer, "missing subscript expression.\n");
    }

    Source_Pos end = lexer->current().end;
    expect(TOKEN_RBRACKET);

    Ast_Subscript *subscript_expr = ast_subscript_expr(op, base, index);
    subscript_expr->mark_range(base->start, end);
    return subscript_expr;
}

Ast_Call *Parser::parse_call_expr(Ast_Expr *expr) {
    Token op = lexer->current();
    expect(TOKEN_LPAREN);

    Auto_Array<Ast_Expr*> arguments;
    do {
        if (lexer->eof() || lexer->match(TOKEN_RPAREN)){
            break;   
        }
        Ast_Expr *arg = parse_expr();
        if (arg == NULL) break;
        arguments.push(arg);
    } while (lexer->eat(TOKEN_COMMA));

    Source_Pos end = lexer->current().end;
    expect(TOKEN_RPAREN);

    Ast_Call *call_expr = ast_call_expr(op, expr, arguments);
    call_expr->mark_range(op.start, end);
    return call_expr;
}

Ast_Expr *Parser::parse_postfix_expr() {
    Ast_Expr *expr = parse_primary_expr();

    if (expr == NULL) return expr;

    bool terminate = false;
    while (!terminate && !lexer->eof()) {
        Token op = lexer->current();
        switch (op.kind) {
        default: terminate = true; break;

        case TOKEN_DOT:
        {
            Ast_Access *access_expr = parse_access_expr(expr);
            expr = access_expr;
            break;
        }

        case TOKEN_DOT_STAR:
        {
            lexer->next_token();
            Ast_Deref *deref_expr = ast_deref_expr(op, expr);
            expr = deref_expr;
            break;
        }

        case TOKEN_LBRACKET:
        {
            Ast_Subscript *subscript_expr = parse_subscript_expr(expr);
            expr = subscript_expr;
            break;
        }

        case TOKEN_LPAREN:
        {
            Ast_Call *call_expr = parse_call_expr(expr);
            expr = call_expr;
            break;
        }
        }
    }

    return expr; 
}

Ast_Expr *Parser::parse_unary_expr() {
    Token op = lexer->current();
    switch (op.kind) {
    default:
    {
        Ast_Expr *expr = parse_postfix_expr();
        return expr;
    }

    case TOKEN_CAST:
    {
        Ast_Type_Defn *type_defn = NULL;
        lexer->next_token();
        if (lexer->eat(TOKEN_LPAREN)) {
            type_defn = parse_type();
            if (type_defn == NULL) {
                report_parser_error(lexer, "missing type in cast expression.\n");
            }
            if (lexer->eat(TOKEN_RPAREN)) {
                Ast_Expr *next_expr = parse_unary_expr();
                if (next_expr) {
                    Ast_Cast *cast = ast_cast_expr(type_defn, next_expr);
                    cast->mark_range(op.start, cast->end);
                    return cast;
                } else {
                    report_parser_error(lexer, "missing expression after cast.\n");
                }
            } else {
                report_parser_error(lexer, "missing ')' after type of cast.\n");
            }
        } else {
            report_parser_error(lexer, "missing '(' after 'cast'.\n");
        }
        return NULL;
        break;
    }

    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_BANG:
    {
        lexer->next_token();
        Ast_Expr *operand = parse_unary_expr();
        Ast_Unary *expr = ast_unary_expr(op, operand);
        return expr;
    }

    case TOKEN_STAR:
    {
        lexer->next_token();
        Ast_Expr *operand = parse_unary_expr();
        Ast_Address *expr = ast_address_expr(op, operand);
        return expr;
    }
    }
}

internal int get_operator_precedence(Token_Kind op) {
    switch (op) {
    default:
        return -1;

    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_MOD:
        return 10000;

    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return 9000;

    case TOKEN_LT:
    case TOKEN_GT:
    case TOKEN_LTEQ:
    case TOKEN_GTEQ:
        return 8000;

    case TOKEN_EQ2:
    case TOKEN_NEQ:
        return 7000;

    case TOKEN_AMPER:
        return 6000;

    case TOKEN_XOR:
        return 5000;
        
    case TOKEN_BAR:
        return 4000;

    case TOKEN_AND:
    case TOKEN_OR:
        return 3000;

    case TOKEN_LSHIFT:
    case TOKEN_RSHIFT:
        return 2000;
    }
}

Ast_Expr *Parser::parse_binary_expr(Ast_Expr *lhs, int current_prec) {
    for (;;) {
        Token op = lexer->current();
        int prec = get_operator_precedence(op.kind);

        if (prec < current_prec) {
            return lhs;
        }

        lexer->next_token();

        Ast_Expr *rhs = parse_unary_expr();
        if (rhs == NULL) {
            report_parser_error(lexer, "expected expression after '%s'.\n", string_from_token(op.kind));
            return lhs;
        }

        Token next_op = lexer->current();
        int next_prec = get_operator_precedence(next_op.kind);

        if (prec < next_prec) {
            rhs = parse_binary_expr(rhs, prec + 1);

            //@Note Missing expression on the right side of the operator
            if (rhs == NULL) {
                rhs = ast_binary_expr(op, lhs, NULL);
                rhs->poison();
                return rhs;
            }
        }

        switch (op.kind) {
        // default:
            // error(op.l0, "invalid operator '%s'.\n", string_from_token(op.kind));
            // break;

        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_MOD:
        case TOKEN_LSHIFT:
        case TOKEN_RSHIFT:
            lhs = ast_arithmetic_expr(op, lhs, rhs);
            break;

        case TOKEN_BAR:
        case TOKEN_AMPER:
        case TOKEN_AND:
        case TOKEN_OR:
            lhs = ast_boolean_expr(op, lhs, rhs);
            break;

        case TOKEN_EQ2:
        case TOKEN_LT:
        case TOKEN_GT:
        case TOKEN_LTEQ:
        case TOKEN_GTEQ:
            lhs = ast_comparison_expr(op, lhs, rhs);
            break;
        }
    }
}

Ast_Expr *Parser::parse_assignment_expr() {
    Ast_Expr *expr = parse_unary_expr();
    if (expr) {
        Token op = lexer->current();
        if (is_assignment_op(op.kind)) {
            lexer->next_token();
            Ast_Expr *rhs = parse_expr();
            expr = ast_assignment_expr(op, expr, rhs);
            if (rhs == NULL) {
                report_parser_error(lexer, "expected expression, got '%s'.\n", string_from_token(lexer->peek()));
                expr->poison();
            }
        }
    }
    return expr;
}

Ast_Expr *Parser::parse_range_expr() {
    Ast_Expr *lhs = parse_expr();

    if (lexer->eat(TOKEN_ELLIPSIS)) {
        Ast_Expr *rhs = parse_expr();
        Ast_Range *range_expr = ast_range_expr(lhs, rhs);
        range_expr->mark_start(lhs->start);
        range_expr->mark_end(lexer->current().start);
        if (rhs) {
            range_expr->mark_end(rhs->end);
        } else {
            report_parser_error(lexer, "missing expression after '..'");
        }
        return range_expr;
    } else {
        return lhs;
    }
}

Ast_Expr *Parser::parse_expr() {
    Ast_Expr *expr = parse_assignment_expr();
    if (expr) {
        expr = parse_binary_expr(expr, 0);
    }
    return expr;
}

Ast_Decl_Stmt *Parser::parse_init_stmt(Ast_Expr *lhs) {
    Ast_Decl_Stmt *stmt = NULL;
    if (lhs->kind == AST_IDENT) {
        Ast_Ident *ident = (Ast_Ident *)lhs;
        Ast_Var *var = parse_var(ident->name);
        var->mark_start(ident->start);
        stmt = ast_decl_stmt(var);
    } else {
        report_parser_error(lexer, "cannot assign to lhs.\n");
        stmt = ast_decl_stmt(NULL);
        stmt->poison();
    }
    return stmt;
}

Ast_Stmt *Parser::parse_simple_stmt() {
    Ast_Stmt *stmt = NULL;
    Ast_Expr *expr = parse_expr();

    if (lexer->match(TOKEN_COLON) || lexer->match(TOKEN_COLON_EQ)) {
        Ast_Decl_Stmt *decl_stmt = parse_init_stmt(expr);
        stmt = decl_stmt;
    } else if (expr) {
        Ast_Expr_Stmt *expr_stmt = ast_expr_stmt(expr);
        stmt = expr_stmt;
    }

    if (stmt &&
        (!expr || expr->invalid())) {
        stmt->poison();
    }

    return stmt;
}

Ast_If *Parser::parse_if_stmt() {
    Ast_If *if_stmt = NULL;
    Source_Pos start = lexer->current().start;

    expect(TOKEN_IF);

    Ast_Expr *cond = parse_expr();
    if (!cond) {
        report_parser_error(lexer, "missing condition for if statement.\n");
        if_stmt->poison();
    }

    Ast_Block *block = parse_block();
    if_stmt = ast_if_stmt(cond, block);

    Ast_If *head = if_stmt;

    Ast_If *tail = head;
    while (lexer->eat(TOKEN_ELSE) && !lexer->eof()) {
        Ast_If *elif = NULL;
        if (lexer->match(TOKEN_IF)) {
            elif = parse_if_stmt();
            elif->if_prev = tail;
            tail->if_next = elif;
        } else {
            Ast_Block *block = parse_block();
            elif = ast_if_stmt(NULL, block);
            elif->is_else = true;
            tail->if_next = elif;
            elif->if_prev = tail;
            tail = elif;
            break;
        }

        if (elif == NULL) break;
        elif->if_prev = tail;
        tail->if_next = elif;
        tail = elif;
    }

    head->mark_range(start, block->end);
    return head;
}

Ast_While *Parser::parse_while_stmt() {
    Source_Pos start = lexer->current().start;
    expect(TOKEN_WHILE);

    Ast_Expr *cond = parse_expr();
    //@Note Infinite loop when while statement has no condition
    if (cond == NULL) {
        Ast_Literal *lit = AST_NEW(Ast_Literal);
        lit->literal_flags = LITERAL_INT;
        lit->int_val = 1;
        cond = lit;
    }

    Ast_Block *block = parse_block();
    Source_Pos end = block->end;
    Ast_While *stmt = ast_while_stmt(cond, block);
    stmt->mark_range(start, end);
    return stmt;
}

//@Todo Change loop syntax from c-like syntax to something better
Ast_For *Parser::parse_for_stmt() {
    Source_Pos start = lexer->current().start;
    expect(TOKEN_FOR);

    Ast_Stmt *init = parse_simple_stmt();

    expect(TOKEN_SEMI);

    Ast_Expr *cond = parse_expr();

    expect(TOKEN_SEMI);

    Ast_Expr *iterator = parse_expr();

    Ast_Block *block = parse_block();

    Ast_For *stmt = ast_for_stmt(init, cond, iterator, block);
    Source_Pos end = block->end;
    stmt->mark_range(start, end);
    return stmt;
}

Ast_Case_Label *Parser::parse_case_label() {
    Source_Pos start = lexer->current().start;

    expect(TOKEN_CASE);
    Ast_Expr *cond = parse_range_expr();

    Source_Pos end = lexer->current().end;
    expect(TOKEN_COLON);

    Ast_Case_Label *label = AST_NEW(Ast_Case_Label);
    label->cond = cond;

    Auto_Array<Ast_Stmt*> fallthrough_statements;

    while (!lexer->eof()) {
        if (lexer->match(TOKEN_CASE)) break;
        Ast_Stmt *stmt = parse_stmt();
        if (!stmt) break;

        if (stmt->kind == AST_FALLTHROUGH) {
            label->fallthrough = true;
        } else {
            label->statements.push(stmt);
        }
        fallthrough_statements.push(stmt);
    }

    if (fallthrough_statements.count) {
        for (Ast_Stmt *stmt : fallthrough_statements) {
            if (stmt->kind == AST_FALLTHROUGH && stmt != fallthrough_statements.back()) {
                report_parser_error(stmt, "illegal #fallthrough, must be placed at end of case block.\n");
            }
        }
        // end = fallthrough_statements.back()->end;
        fallthrough_statements.clear();
    }

    label->mark_range(start, end);

    return label;
}

Ast_Ifcase *Parser::parse_ifcase_stmt() {
    Token token = lexer->current();
    expect(TOKEN_IFCASE);

    Ast_Ifcase *ifcase = AST_NEW(Ast_Ifcase);

    Ast_Expr *cond = parse_expr();
    ifcase->cond = cond;

    expect(TOKEN_LBRACE);

    Ast_Case_Label *last_label = nullptr;

    while (!lexer->eof()) {
        if (lexer->match(TOKEN_RBRACE)) break;

        Ast_Stmt *stmt = parse_stmt();
        if (!stmt) break;

        if (stmt->kind == AST_CASE_LABEL) {
            Ast_Case_Label *label = static_cast<Ast_Case_Label*>(stmt);

            if (last_label) {
                last_label->next_label = label;
            }
            label->prev_label = last_label;
            last_label = label;

            ifcase->cases.push(label);
        } else {
            report_parser_error(lexer, "illegal statement in case statement, not under a case label.\n");
            ifcase->poison();
        }
    }

    Source_Pos end = lexer->current().end;
    expect(TOKEN_RBRACE);

    ifcase->mark_range(token.start, end);
    return ifcase;
}

Ast_Stmt *Parser::parse_stmt() {
    Ast_Stmt *stmt = NULL;
    bool stmt_error = false;

    switch (lexer->peek()) {
    default:
    {
        stmt = parse_simple_stmt();
        if (stmt) {
            if (!lexer->eat(TOKEN_SEMI)) {
                report_parser_error(lexer, "expected ';', got '%s'.\n", string_from_token(lexer->peek()));
                stmt_error = true;
            }
        }
        break;
    }

    case TOKEN_THROUGH:
    {
        Token token = lexer->current();
        lexer->next_token();
        stmt = AST_NEW(Ast_Fallthrough);
        stmt->mark_range(token.start, token.end);
        break;
    }

    case TOKEN_SEMI:
    {
        lexer->next_token(); 
        stmt = AST_NEW(Ast_Stmt);
        break;
    }

    case TOKEN_LBRACE:
    {
        Ast_Block *block = parse_block();
        stmt = block;
        stmt->mark_range(block->start, block->end);
        break;
    }

    case TOKEN_CASE:
    {
        Ast_Case_Label *case_label = parse_case_label();
        stmt = case_label;
        break;
    }

    case TOKEN_IFCASE:
    {
        Ast_Ifcase *ifcase = parse_ifcase_stmt();
        stmt = ifcase;
        break;
    }

    case TOKEN_IF:
    {
        Ast_If *if_stmt = parse_if_stmt();
        stmt = if_stmt;
        break;
    }
    case TOKEN_ELSE:
    {
        report_parser_error(lexer, "illegal else without matching if.\n");
        stmt_error = true;
        break;
    }

    case TOKEN_WHILE:
    {
        Ast_While *while_stmt = parse_while_stmt();
        stmt = while_stmt;
        break;
    }

    case TOKEN_FOR:
    {
        Ast_For *for_stmt = parse_for_stmt();
        stmt = for_stmt;
        break;
    }

    case TOKEN_BREAK:
    {
        lexer->next_token();
        Ast_Break *break_stmt = AST_NEW(Ast_Break);
        stmt = break_stmt;
        break;
    }

    case TOKEN_CONTINUE:
    {
        lexer->next_token();
        Ast_Continue *continue_stmt = AST_NEW(Ast_Continue);
        stmt = continue_stmt;
        break;
    }

    case TOKEN_RETURN:
    {
        lexer->next_token();
        Ast_Expr *expr = parse_expr();
        expect(TOKEN_SEMI);
        Ast_Return *return_stmt = ast_return(expr);
        stmt = return_stmt;
        break;
    }
    }

    //@Note Try to setup the next statement to parse or get to end of the block
    if (stmt_error) {
        while (!lexer->eof()) {
            if (lexer->eat(TOKEN_SEMI)) {
                break;
            }
            if (lexer->match(TOKEN_RBRACE)) {
                break;
            }

            lexer->next_token();
        }
    }

    return stmt;
}

Ast_Type_Defn *Parser::parse_type() {
    Ast_Type_Defn *type = NULL;

    bool terminate = false;
    while (!terminate && !lexer->eof()) {
        switch (lexer->peek()) {
        default:
            terminate = true;
            break;
            
        case TOKEN_IDENT:
        {
            Token name = lexer->current();
            lexer->next_token();
            
            Ast_Type_Defn *t = ast_type_defn(TYPE_DEFN_NAME, type);
            t->name = name.name;
            t->mark_range(name.start, name.end);
            type = t;

            terminate = true;
            break;
        }

        case TOKEN_STAR:
        {
            Token op = lexer->current();
            lexer->next_token();
            Ast_Type_Defn *t = ast_type_defn(TYPE_DEFN_POINTER, type);
            t->mark_start(op.start);
            type = t;
            break;
        }

        case TOKEN_LBRACKET:
        {
            Token op = lexer->current();
            lexer->next_token();
            Ast_Expr *array_size = parse_expr();
            expect(TOKEN_RBRACKET);
            Ast_Type_Defn *t = ast_type_defn(TYPE_DEFN_ARRAY, type);
            t->array_size = array_size;
            t->mark_start(op.start);
            type = t;
            break;
        }
        }
    }

    if (type && type->type_defn_kind != TYPE_DEFN_NAME) {
        report_parser_error(lexer, "expected a type, got '%s'.\n", string_from_token(lexer->peek()));
        type->poison();
    }
    return type;
}

Ast_Param *Parser::parse_param() {
    Ast_Param *param = NULL;
    if (lexer->match(TOKEN_IDENT)) {
            Token name = lexer->current();
            lexer->next_token();
            expect(TOKEN_COLON);
            Ast_Type_Defn *type_defn = parse_type();
            if (!type_defn) {
                report_parser_error(lexer, "expected type after ':', got '%s'.\n", string_from_token(lexer->peek()));
            }
            param = ast_param(name.name, type_defn);
    } else if (lexer->match(TOKEN_ELLIPSIS)) {
        lexer->next_token();
        param = ast_param(NULL, NULL);
        param->is_vararg = true;
    }
    return param;
}

Ast_Block *Parser::parse_block() {
    Source_Pos start = lexer->current().start;
    Ast_Block *block = AST_NEW(Ast_Block);
    expect(TOKEN_LBRACE);
    while (!lexer->eof() && !lexer->match(TOKEN_RBRACE)) {
        Ast_Stmt *stmt = parse_stmt();
        if (stmt == NULL) break;
        block->statements.push(stmt);
    }
    Source_Pos end = lexer->current().end;
    expect(TOKEN_RBRACE);
    block->mark_range(start, end);
    return block;
}

Ast_Proc *Parser::parse_proc(Token name) {
    bool has_varargs = false;
    Auto_Array<Ast_Param*> parameters;
    expect(TOKEN_LPAREN);
    while (!lexer->eof() && !lexer->match(TOKEN_RPAREN)) {
        Ast_Param *param = parse_param();
        if (param == NULL) break;
        parameters.push(param);

        if (has_varargs && param->is_vararg) {
            report_parser_error(lexer, "Variadic parameter must be used only once.\n");
        } else if (has_varargs) {
            report_parser_error(lexer, "Variadic parameter must be last.\n");
        }

        if (param->is_vararg) has_varargs = true;

        if (!lexer->eat(TOKEN_COMMA)) {
            break;
        }
    }
    expect(TOKEN_RPAREN);

    Ast_Type_Defn *return_type = NULL;
    if (lexer->eat(TOKEN_ARROW)) {
        return_type = parse_type();
    }

    bool is_foreign = false;
    Ast_Block *block = NULL;
    if (lexer->match(TOKEN_LBRACE)) {
        block = parse_block();
    } else if (lexer->eat(TOKEN_FOREIGN)) {
        is_foreign = true;
    }
    Source_Pos end = lexer->current().start;
    
    Ast_Proc *proc = ast_proc(name.name, parameters, return_type, block);
    proc->foreign = is_foreign;
    proc->has_varargs = has_varargs;
    proc->mark_range(name.start, end);
    return proc;
}

Ast_Operator_Proc *Parser::parse_operator_proc() {
    Source_Pos start = lexer->current().start;
    
    expect(TOKEN_OPERATOR);

    Ast_Operator_Proc *proc = NULL;

    Token op = lexer->current();

    Auto_Array<Ast_Param*> parameters;

    if (is_operator(op.kind)) {
        lexer->next_token();

        if (op.kind == TOKEN_LBRACKET) {
            expect(TOKEN_RBRACKET);
        }

        if (!lexer->eat(TOKEN_COLON2)) {
            report_parser_error(lexer, "missing '::', got '%s'.\n", string_from_token(lexer->peek()));
        }

        if (operator_is_overloadable(op.kind)) {
            if (!lexer->eat(TOKEN_LPAREN)) {
                report_parser_error(lexer, "missing '('.\n");
                goto ERROR_HANDLE;
            }

            while (!lexer->eof() && !lexer->match(TOKEN_RPAREN)) {
                Ast_Param *param = parse_param();
                if (param == NULL) break;
                parameters.push(param);
                if (!lexer->eat(TOKEN_COMMA)) {
                    break;
                }
            }

            Source_Pos end = lexer->current().end;

            if (!lexer->eat(TOKEN_RPAREN)) {
                report_parser_error(lexer, "missing ')'.\n");
                goto ERROR_HANDLE;
            }

            Ast_Type_Defn *return_type = NULL;
            if (lexer->eat(TOKEN_ARROW)) {
                return_type = parse_type();
            }

            Ast_Block *block = parse_block();
            proc = ast_operator_proc(op.kind, parameters, return_type, block);
            proc->mark_range(start, end);
        } else {
            report_parser_error(lexer, "invalid operator, cannot overload '%s'.\n", string_from_token(op.kind));
            goto ERROR_HANDLE;
        }
    } else {
        report_parser_error(lexer, "expected operator, got '%s'.\n", string_from_token(op.kind));
        goto ERROR_HANDLE;
    } 

    return proc;

ERROR_HANDLE:
    return NULL;
}

Ast_Struct_Field *Parser::parse_struct_field() {
    Ast_Struct_Field *field = NULL;
    Token name = lexer->current();
    if (lexer->eat(TOKEN_IDENT)) {
        expect(TOKEN_COLON);
        Ast_Type_Defn *type_defn = parse_type();
        expect(TOKEN_SEMI);
        field = ast_struct_field(name.name, type_defn);
        field->mark_range(name.start, type_defn->end);
    }
    return field;
}

Ast_Struct *Parser::parse_struct(Token name) {
    expect(TOKEN_STRUCT);

    Source_Pos end = {};
    Auto_Array<Ast_Struct_Field*> fields;
    if (lexer->eat(TOKEN_LBRACE)) {
        while (!lexer->match(TOKEN_RBRACE) && !lexer->eof()) {
            Ast_Struct_Field *field = parse_struct_field();
            if (field == NULL) break;
            fields.push(field);
        }

        end = lexer->current().end;
        expect(TOKEN_RBRACE);
    }

    Ast_Struct *struct_decl = ast_struct(name.name, fields);
    struct_decl->mark_range(name.start, end);
    return struct_decl;
}

Ast_Enum_Field *Parser::parse_enum_field() {
    Ast_Enum_Field *field = NULL;
    Token name = lexer->current();
    if (lexer->eat(TOKEN_IDENT)) {
        field = ast_enum_field(name.name);
        field->mark_range(name.start, name.end);
    }
    return field;
}

Ast_Enum *Parser::parse_enum(Token name) {
    lexer->eat(TOKEN_ENUM);

    Source_Pos end = {};
    Auto_Array<Ast_Enum_Field*> fields;
    if (lexer->eat(TOKEN_LBRACE)) {
        while (!lexer->match(TOKEN_RBRACE) && !lexer->eof()) {
            Ast_Enum_Field *field = parse_enum_field();
            if (field == NULL) break;
            
            fields.push(field);

            if (!lexer->eat(TOKEN_COMMA)) {
                break; 
            }
        }

        end = lexer->current().end;
        expect(TOKEN_RBRACE);
    }

    Ast_Enum *enum_decl = ast_enum(name.name, fields);
    enum_decl->mark_range(name.start, end);
    return enum_decl;
}

Ast_Decl *Parser::parse_decl() {
    Ast_Decl *decl = NULL;
    Token token = lexer->current();
    if (lexer->eat(TOKEN_IDENT)) {
        if (lexer->eat(TOKEN_COLON2)) {
            switch (lexer->peek()) {
            case TOKEN_STRUCT:
            {
                Ast_Struct *struct_decl = parse_struct(token);
                decl = struct_decl;
                break;
            }
            case TOKEN_ENUM:
            {
                Ast_Enum *enum_decl = parse_enum(token);
                decl = enum_decl;
                break;
            }
            case TOKEN_LPAREN:
            {
                Ast_Proc *proc = parse_proc(token);
                decl = proc;
                break;
            }
            }
        } else if (lexer->match(TOKEN_COLON)) {
            Ast_Var *var = parse_var(token.name);
            var->mark_start(token.start);
            expect(TOKEN_SEMI);
            decl = var;
        }
    } else if (lexer->match(TOKEN_OPERATOR)) {
        Ast_Operator_Proc *proc = parse_operator_proc();
        decl = proc;
    }
    return decl;
}

Ast_Var *Parser::parse_var(Atom *name) {
    Ast_Expr *init = NULL;
    Ast_Type_Defn *type_defn = NULL;

    if (lexer->eat(TOKEN_COLON)) {
        type_defn = parse_type();
        if (!type_defn) {
            report_parser_error(lexer, "expected type after ':'.\n");
            goto ERROR_BLOCK;
        }
        if (lexer->eat(TOKEN_EQ)) {
            init = parse_expr();
            if (!init) {
                report_parser_error(lexer, "expected expression after '='.\n");
                goto ERROR_BLOCK;
            }
        }
    } else if (lexer->eat(TOKEN_COLON_EQ)) {
        init = parse_expr();
        if (!init) {
            report_parser_error(lexer, "expected expression after ':='.\n");
            goto ERROR_BLOCK;
        }
    } else {
        Assert(0);
    }

    Ast_Var *var = ast_var(name, init, type_defn);
    if (var->init) {
        var->mark_end(init->end);
    } else {
        var->mark_end(type_defn->end);
    }
    return var;

ERROR_BLOCK:
    Ast_Var *err = ast_var(name, init, type_defn);
    err->poison();
    return err;
}

void Parser::parse_load_directive() {
    lexer->next_token();
    if (lexer->match(TOKEN_STRLIT)) {
        String8 file_path = lexer->current().strlit;
        lexer->next_token();

        if (path_is_relative(file_path)) {
            String8 current_dir = path_dir_name(lexer->source_file->path);
            file_path = path_join(heap_allocator(), current_dir, file_path);
        }

        Source_File *file = source_file_create(file_path);
        add_source_file(file);
    } else {
        report_parser_error(lexer, "expected filename.\n");
    }

}

void Parser::parse() {
    root = AST_NEW(Ast_Root);

    for (;;) {
        if (lexer->eof()) {
            if (!load_next_source_file()) {
                return;
            }
        }

        switch (lexer->peek()) {
        default:
        {
            Ast_Decl *decl = parse_decl();
            if (decl) {
                root->declarations.push(decl);
            } else {
                printf("recovering from decl, curr: %s.\n", string_from_token(lexer->peek()));
                while (!lexer->eof()) {
                    Token token = lexer->current();
                    if (token.kind == TOKEN_RBRACE && token.start.col == 0) {
                        lexer->next_token();
                        break;
                    }
                    lexer->next_token();
                }
            }
            break;
        }

        case TOKEN_LOAD:
        {
            parse_load_directive();
            break;
        }
        }
    }
}
