//@Todo Fix paren expression parsing. Conflicts with procedure type.
//@Todo Fix compound literal parsing. Conflicts with control-statements ?

#include "path/path.h"

#include "atom.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "report.h"

#include "os/os.h"

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

Array<Ast*> Parser::parse_expr_list() {
    auto list = array_make<Ast*>(heap_allocator());

    Ast *expr = parse_expr();
    if (!expr) {
        return list;
    }
    array_add(&list, expr);
    while (lexer->eat(TOKEN_COMMA)) {
        expr = parse_expr();
        if (!expr) {
            report_parser_error(lexer, "expected expression after ','.\n");
            break;
        }
        array_add(&list, expr);
    }
    
    return list;
}

Array<Ast*> Parser::parse_type_list() {
    auto type_list = array_make<Ast*>(heap_allocator());

    for (;;) {
        Ast *type = parse_type();
        if (!type) {
            break;
        }

        array_add(&type_list, type);

        if (!lexer->eat(TOKEN_COMMA)) {
            break; 
        }
    }

    return type_list;
}

Ast_Compound_Literal *Parser::parse_compound_literal(Token token, Ast *type) {
    Token open = expect_token(TOKEN_LBRACE);

    auto elements = array_make<Ast*>(heap_allocator());

    while (!lexer->match(TOKEN_RBRACE)) {
        Ast *expr = parse_expr(); 
        if (expr == NULL) break;

        array_add(&elements, expr);

        if (!lexer->eat(TOKEN_COMMA)) break;
    }

    Token close = expect_token(TOKEN_RBRACE);

    Ast_Compound_Literal *compound = ast_compound_literal(file, token, open, close, type, elements);
    return compound;
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

    auto arguments = array_make<Ast*>(heap_allocator());
    do {
        if (lexer->match(TOKEN_RPAREN)) {
            break;   
        }
        Ast *arg = parse_expr();
        if (!arg) break;
        array_add(&arguments, arg);
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
    Token token = lexer->current();

    //@Note Anonymous members
    if (token.kind == TOKEN_STRUCT || token.kind == TOKEN_ENUM || token.kind == TOKEN_UNION) {
        Ast *operand = parse_operand();

        auto values = array_make<Ast*>(heap_allocator(), {operand});

        bool is_mutable = true;
        return ast_value_decl(file, {}, nullptr, values, is_mutable);
    }

    Array<Ast*> lhs = parse_expr_list();
    auto values = array_make<Ast*>(heap_allocator());
    Ast *type = nullptr;

    expect_token(TOKEN_COLON);

    return parse_value_decl(lhs);
}

Array<Ast_Value_Decl*> Parser::parse_struct_members() {
    auto members = array_make<Ast_Value_Decl*>(heap_allocator());
    while (!lexer->match(TOKEN_RBRACE)) {
        Ast_Value_Decl *member = parse_struct_member();
        if (!member) break;
        array_add(&members, member);

        if (member->is_mutable && member->names.count > 0) {
            expect_semi();
        }
    }
    return members;
}

Array<Ast_Enum_Field*> Parser::parse_enum_field_list() {
    auto field_list = array_make<Ast_Enum_Field*>(heap_allocator());

    while (!lexer->match(TOKEN_RBRACE)) {
        Ast_Ident *ident = parse_ident();

        Ast *expr = nullptr;
        if (lexer->eat(TOKEN_EQ)) {
            expr = parse_expr();
        }

        Ast_Enum_Field *enum_field = ast_enum_field(file, ident, expr);
        array_add(&field_list, enum_field);

        if (!lexer->eat(TOKEN_COMMA)) {
            break; 
        }
    }

    return field_list;
}

Ast_Paren *Parser::parse_paren_expr() {
    Token open = expect_token(TOKEN_LPAREN);
    Ast *elem = parse_expr();
    if (!elem) {
        report_parser_error(lexer, "missing expression in paren.\n");
    }
    Token close = expect_token(TOKEN_RPAREN);
    return ast_paren_expr(file, open, close, elem);
}

Ast_Selector *Parser::parse_selector_expr(Token token, Ast *base) {
    Token name = lexer->current();
    if (!lexer->eat(TOKEN_IDENT)) {
        report_parser_error(lexer, "missing name after '.'\n");
    }
    return ast_selector_expr(file, token, base, ast_ident(file, name));
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

        case TOKEN_DOT: {
            Token token = expect_token(TOKEN_DOT);
            if (lexer->match(TOKEN_LBRACE)) {
                operand = parse_compound_literal(token, operand);
            } else {
                operand = parse_selector_expr(token, operand);
            }
            break;
        }

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
        Ast *array_size = nullptr;
        bool is_dynamic = false;
        if (lexer->eat(TOKEN_ELLIPSIS)) {
            is_dynamic = true;
        } else {
            array_size = parse_expr();
        }
        expect_token(TOKEN_RBRACKET);
        Ast *type = parse_type();
        if (!type) {
            report_parser_error(lexer, "missing type of array or slice type.\n");
        }

        Ast_Array_Type *array_type = ast_array_type(file, token, type, array_size);
        array_type->is_dynamic = is_dynamic;
        array_type->array_size = array_size;
        if (!is_dynamic && array_size == nullptr) {
            array_type->is_view = true; 
        }
        return array_type;
    }

    case TOKEN_LPAREN: {
        if (!allow_value_decl) {
            return parse_paren_expr();
        }

        Ast_Proc_Type *type = parse_proc_type();

        while (lexer->eat(TOKEN_HASH)) {
            Token tag = expect_token(TOKEN_IDENT);
            if (tag.string == "foreign") {
                type->is_foreign = true;
            } else {
                report_parser_error(lexer, "unknown procedure directive '%S'.\n", tag.string); 
            }
        }

        if (allow_type) {
            return type;
        }

        if (lexer->match(TOKEN_LBRACE)) {
            Ast_Block *body = parse_block();
            return ast_proc_lit(file, type, body);
        } else if (lexer->eat(TOKEN_UNINIT)) {
            return ast_proc_lit(file, type, nullptr);
        } else {
            return type;
        }
    }

    case TOKEN_UNION:
    case TOKEN_STRUCT: {
        lexer->next_token();

        Token open = lexer->current();
        Token close = lexer->current();

        auto members = array_make<Ast_Value_Decl*>(heap_allocator());
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

        Array<Ast_Enum_Field*> field_list = parse_enum_field_list();

        Token close = expect_token(TOKEN_RBRACE);

        return ast_enum_type(file, token, open, close, base_type, field_list);
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

    case TOKEN_SIZEOF: {
        expect_token(TOKEN_SIZEOF);
        Ast *elem = parse_unary_expr();
        Ast_Sizeof *expr = ast_sizeof_expr(file, token, elem);
        return expr;
    }
        
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_BANG:
    case TOKEN_SQUIGGLE: {
        lexer->next_token();
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

    auto statements = array_make<Ast*>(heap_allocator());
    while (!lexer->match(TOKEN_RBRACE)) {
        Ast *stmt = parse_stmt();
        if (stmt == NULL) break;
        array_add(&statements, stmt);
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

    bool is_range = false;

    Ast *init = nullptr;
    Ast *condition = nullptr;
    Ast *post = nullptr;
    Ast_Block *block = nullptr;

    //@Note for {..}
    if (lexer->match(TOKEN_LBRACE)) {
        Ast_Block *block = parse_block();
        return ast_for_stmt(file, token, nullptr, nullptr, nullptr, block);
    }

    init = parse_simple_stmt();
    if (init && init->kind == AST_ASSIGNMENT) {
        Ast_Assignment *assignment = (Ast_Assignment *)init;
        if (assignment->op == OP_IN) {
            is_range = true;
        }
    }

    if (!is_range) {
        expect_semi();

        condition = parse_expr();

        expect_semi();

        post = parse_simple_stmt();
    }


    if (lexer->match(TOKEN_LBRACE)) {
        block = parse_block();
    } else {
        report_parser_error(lexer, "missing for statement block.\n");
    }

    if (is_range) {
        return ast_range_stmt(file, token, (Ast_Assignment *)init, block);
    }

    return ast_for_stmt(file, token, init, condition, post, block);
}

Ast_Case_Label *Parser::parse_case_clause() {
    Token token = expect_token(TOKEN_CASE);

    Ast *cond = parse_range_expr();

    expect_token(TOKEN_COLON);

    auto statements = array_make<Ast*>(heap_allocator());

    for (;;) {
        Ast *stmt = parse_stmt();
        if (!stmt) break;
        array_add(&statements, stmt);
    }

    for (Ast *stmt : statements) {
        if (stmt->kind == AST_FALLTHROUGH) {
            if (stmt != array_back(statements)) {
                report_ast_error(stmt, "illegal fallthrough, must be placed at end of a case block.\n");
            }
        }
    }

    return ast_case_label(file, token, cond, statements);
}

Ast_Ifcase *Parser::parse_ifcase_stmt() {
    Token token = lexer->current();
    expect_token(TOKEN_IFCASE);

    bool check_complete = false;

    while (lexer->eat(TOKEN_HASH)) {
        Token tag = expect_token(TOKEN_IDENT);
        if (tag.string == "complete") {
            check_complete = true;
        } else {
            report_parser_error(lexer, "unknown directive tag for ifcase statement '%S'.\n", tag.string);
        }
    }

    Ast *cond = parse_expr();

    Token open = expect_token(TOKEN_LBRACE);

    auto clauses = array_make<Ast_Case_Label*>(heap_allocator());

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

        array_add(&clauses, case_clause);
    }

    Token close = expect_token(TOKEN_RBRACE);

    return ast_ifcase_stmt(file, token, open, close, cond, clauses, check_complete);
}

Ast *Parser::parse_load_stmt(Token token) {
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

    if (!source_file) {
        report_line("path does not exit: %s", file_path.data);
        return ast_bad_stmt(file, file_token, file_token);
    }

    expect_semi();

    add_source_file(source_file);

    return ast_load_stmt(file, token, file_token, file_path);
}

Ast *Parser::parse_import_stmt(Token token) {
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

    if (!source_file) {
        report_line("path does not exit: %s", file_path.data);
        return ast_bad_stmt(file, file_token, file_token);
    }

    expect_semi();

    add_source_file(source_file);

    return ast_import_stmt(file, token, file_token, file_path);
}

Ast_Proc_Type *Parser::parse_proc_type() {
    auto params = array_make<Ast_Param*>(heap_allocator());

    bool has_varargs = false;

    Token open = expect_token(TOKEN_LPAREN);

    while (!lexer->match(TOKEN_RPAREN)) {
        Ast_Ident *ident = parse_ident();

        expect_token(TOKEN_COLON);

        bool is_variadic = false;
        if (lexer->match(TOKEN_ELLIPSIS)) {
            Token ellipsis = expect_token(TOKEN_ELLIPSIS);
            is_variadic = true;
            has_varargs = true;
        }

        Ast *type = parse_type();

        if (is_variadic && !type) {
            report_parser_error(lexer, "missing type after variadic field prefix '..'.\n");
        }

        Ast_Param *param = ast_param(file, ident, type, is_variadic);
        array_add(&params, param);

        if (!lexer->eat(TOKEN_COMMA)) {
            break;
        }
    }

    Token close = expect_token(TOKEN_RPAREN);

    auto return_types = array_make<Ast*>(heap_allocator());
    if (lexer->eat(TOKEN_ARROW)) {
        return_types = parse_type_list();
    }

    return ast_proc_type(file, open, close, params, return_types, has_varargs);
}

Ast *Parser::parse_type() {
    bool prev_allow_type = allow_type;
    allow_type = true;

    Ast *type = parse_operand();
    while (lexer->match(TOKEN_DOT)) {
        Token token = expect_token(TOKEN_DOT);
        Ast_Selector *selector = parse_selector_expr(token, type);
        type = selector;
    }

    allow_type = prev_allow_type;
    return type;
}

Ast_Value_Decl *Parser::parse_value_decl(Array<Ast*> names) {
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

    auto values = array_make<Ast*>(heap_allocator());
    
    if (token.kind == TOKEN_COLON || token.kind == TOKEN_EQ) {
        lexer->next_token();
        is_mutable = token.kind != TOKEN_COLON;

        allow_value_decl = true;

        values = parse_expr_list();

        allow_value_decl = false;
    }

    return ast_value_decl(file, names, type, values, is_mutable);
}

Ast *Parser::parse_simple_stmt() {
    Array<Ast*> lhs = parse_expr_list();

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
        Array<Ast*> rhs = parse_expr_list();
        if (rhs.count == 0) {
            report_parser_error(lexer, "missing rhs in assignment statement.\n");
            return ast_bad_stmt(file, token, lexer->current());
        }
        Ast_Assignment *assignment = ast_assignment_stmt(file, token, op, lhs, rhs);
        return assignment;
    }

    case TOKEN_COLON: {
        expect_token(TOKEN_COLON);
        Ast_Value_Decl *value_decl = parse_value_decl(lhs);
        return value_decl;
    }

    case TOKEN_IN: {
        expect_token(TOKEN_IN);
        Ast *expr = parse_range_expr();
        Array<Ast*> rhs = array_make<Ast*>(heap_allocator(), {expr});
        Ast_Assignment *assignment = ast_assignment_stmt(file, token, OP_IN, lhs, rhs);
        return assignment;
    }
    }

    if (lhs.count > 1) {
        report_parser_error(lexer, "expected just one expression.\n");
    }

    Ast_Stmt *expr_stmt = ast_expr_stmt(file, array_front(lhs));
    return expr_stmt;
}

void Parser::expect_semi() {
    expect_token(TOKEN_SEMI);
}

Ast_Return *Parser::parse_return_stmt() {
    Token token = expect_token(TOKEN_RETURN);

    Array<Ast*> values = parse_expr_list();

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

    case TOKEN_HASH: {
        expect_token(TOKEN_HASH);
        Token directive = expect_token(TOKEN_IDENT);
        if (directive.string == "load") {
            return parse_load_stmt(token);
        } else if (directive.string == "import") {
            return parse_import_stmt(token);
        } else {
            report_parser_error(lexer, "unknown directive '%S'.\n", directive.string);
            return ast_bad_stmt(file, lexer->current(), lexer->current());
        }
        break;
    }
    }

    stmt = parse_simple_stmt();

    if (stmt) {
        if (stmt->kind == AST_EXPR_STMT || stmt->kind == AST_ASSIGNMENT) {
            expect_semi();
        } else if (stmt->kind == AST_VALUE_DECL) {
            Ast_Value_Decl *vd = static_cast<Ast_Value_Decl*>(stmt);
            if (vd->is_mutable) {
                expect_semi();
            }
        }
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
    auto decls = array_make<Ast*>(heap_allocator());

    for (;;) {
        if (lexer->eof()) {
            if (!load_next_source_file()) {
                break;
            }
        }

        Ast *stmt = parse_stmt();
        if (stmt && stmt->kind != AST_EMPTY_STMT) {
            array_add(&decls, (Ast*)stmt);
        }
    }

    root = ast_root(file, decls);
}
