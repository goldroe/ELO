#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "parser.h"

char *type_definition_to_string(Ast_Type_Definition *type_definition) {
    static char buffer[1024];
    memset(buffer, 0, 1024);
    for (Ast_Type_Definition *it = type_definition; it; it = it->base) {
        if (it->type_flags & TYPE_DEFINITION_POINTER) strcat(buffer, "*");
        else if (it->type_flags & TYPE_DEFINITION_ARRAY) strcat(buffer, "[]");
        else if (it->type_flags & TYPE_DEFINITION_IDENT) strcat(buffer, it->ident->name->name);
    }
    return buffer;
}

void Parser::error(const char *fmt, ...) {
    printf("error: ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

bool Parser::expect(Token_Type type) {
    if (lexer->is_token(type)) {
        lexer->next_token();
        return true;
    }
    error("expected '%s', got '%s'", token_type_to_string(type), token_type_to_string(lexer->token.type));
    return false;
}

static inline Ast *allocate_ast_node(size_t size) {
    Ast *result = (Ast *)calloc(size, 1);
    return result;
}
#define AST_NEW(AST_T) (AST_T *)(&(*allocate_ast_node(sizeof(AST_T)) = AST_T()))

static inline Ast_Root *make_root() {
    Ast_Root *root = AST_NEW(Ast_Root);
    return root;
}

static inline Ast_Ident *make_ident(Atom *name) {
    Ast_Ident *ident = AST_NEW(Ast_Ident);
    ident->name = name;
    return ident;
}

static inline Ast_Procedure_Declaration *make_procedure_declaration(Ast_Ident *ident) {
    Ast_Procedure_Declaration *procedure = AST_NEW(Ast_Procedure_Declaration);
    procedure->ident = ident;
    return procedure;
}

static inline Ast_Declaration_Statement *make_declaration_statement(Ast_Declaration *declaration) {
    Ast_Declaration_Statement *declaration_statement = AST_NEW(Ast_Declaration_Statement);
    declaration_statement->declaration = declaration;
    return declaration_statement;
}

static inline Ast_Expression_Statement *make_expression_statement(Ast_Expression *expression) {
    Ast_Expression_Statement *expression_statement = AST_NEW(Ast_Expression_Statement);
    expression_statement->expression = expression;
    return expression_statement;
}

static inline Ast_Block *make_block() {
    Ast_Block *block = AST_NEW(Ast_Block);
    return block;
}

static inline Ast_Literal *make_integer_literal(int64 value) {
    Ast_Literal *literal = AST_NEW(Ast_Literal);
    literal->literal_flags |= LITERAL_NUMBER;
    literal->int_value = value;
    return literal;
}

static inline Ast_Literal *make_float_literal(float64 value) {
    Ast_Literal *literal = AST_NEW(Ast_Literal);
    literal->literal_flags |= LITERAL_NUMBER;
    literal->literal_flags |= LITERAL_FLOAT;
    literal->float_value = value;
    return literal;
}

static inline Ast_Literal *make_string_literal(char *value) {
    Ast_Literal *literal = AST_NEW(Ast_Literal);
    literal->literal_flags |= LITERAL_STRING;
    literal->string_value = value;
    return literal;
}

static inline Ast_Variable *make_variable_declaration(Ast_Ident *ident) {
    Ast_Variable *variable = AST_NEW(Ast_Variable);
    variable->ident = ident;
    return variable;
}

static inline Ast_Type_Definition *make_type_definition(Ast_Type_Definition *base) {
    Ast_Type_Definition *type_definition = AST_NEW(Ast_Type_Definition);
    type_definition->base = base;
    return type_definition;
}

static inline Ast_Unary_Expression *make_unary_expression(Token_Type op) {
    Ast_Unary_Expression *unary = AST_NEW(Ast_Unary_Expression);
    unary->op = op;
    return unary;
}

static inline Ast_Binary_Expression *make_binary_expression(Token_Type op, Ast_Expression *lhs, Ast_Expression *rhs) {
    Ast_Binary_Expression *binary_expression = AST_NEW(Ast_Binary_Expression);
    binary_expression->op = op;
    binary_expression->lhs = lhs;
    binary_expression->rhs = rhs;
    return binary_expression;
}

static inline Ast_Index_Expression *make_index_expression(Ast_Expression *array, Ast_Expression *index) {
    Ast_Index_Expression *index_expression = AST_NEW(Ast_Index_Expression);
    index_expression->array = array;
    index_expression->index = index;
    return index_expression;
}

static inline Ast_Field_Expression *make_field_expression(Ast_Expression *operand, Ast_Expression *field) {
    Ast_Field_Expression *field_expression = AST_NEW(Ast_Field_Expression);
    field_expression->operand = operand;
    field_expression->field = field;
    return field_expression;
}

static inline Ast_Call_Expression *make_call_expression(Ast_Expression *operand) {
    Ast_Call_Expression *call_expression = AST_NEW(Ast_Call_Expression);
    call_expression->operand = operand;
    return call_expression;
}

static inline Ast_If *make_if_statement(Ast_Expression *condition, Ast_Block *block) {
    Ast_If *if_statement = AST_NEW(Ast_If);
    if_statement->condition = condition;
    if_statement->block = block;
    return if_statement;
}

static inline Ast_While *make_while_statement(Ast_Expression *condition, Ast_Block *block) {
    Ast_While *while_statement = AST_NEW(Ast_While);
    while_statement->condition = condition;
    while_statement->block = block;
    return while_statement;
}

static inline Ast_Return *make_return_statement(Ast_Expression *expression) {
    Ast_Return *return_statement = AST_NEW(Ast_Return);
    return_statement->expression = expression;
    return return_statement;
}

static inline Ast_Struct_Declaration *make_struct_declaration(Ast_Ident *ident) {
    Ast_Struct_Declaration *declaration = AST_NEW(Ast_Struct_Declaration);
    declaration->ident = ident;
    return declaration;
}

static inline Ast_Struct_Field *make_struct_field(Ast_Ident *ident, Ast_Type_Definition *type_definition) {
    Ast_Struct_Field *field = AST_NEW(Ast_Struct_Field);
    field->ident = ident;
    field->type_definition = type_definition;
    return field;
}

Ast_Expression *Parser::parse_operand() {
    switch (lexer->token.type) {
    case TOKEN_IDENT:
    {
        Ast_Ident *ident = make_ident(lexer->token.name);
        lexer->next_token();
        return ident;
    }
    case TOKEN_INTLIT: 
    {
        Ast_Literal *literal = make_integer_literal(lexer->token.intlit);
        lexer->next_token();
        return literal;
    }
    case TOKEN_FLOATLIT:
    {
        Ast_Literal *literal = make_float_literal(lexer->token.floatlit);
        lexer->next_token();
        return literal;
    }
    case TOKEN_STRLIT:
    {
        Ast_Literal *literal = make_string_literal(lexer->token.strlit);
        lexer->next_token();
        return literal;
    }
    }
    return nullptr;
}

Ast_Expression *Parser::parse_primary_expression() {
    Ast_Expression *operand = parse_operand();
    switch (lexer->token.type) {
    default:
        return operand;
    case TOKEN_LBRACKET:
    {
        lexer->next_token();
        Ast_Expression *index = parse_expression();
        Ast_Index_Expression *index_expression = make_index_expression(operand, index);
        expect(TOKEN_RBRACKET);
        return index_expression;
    }
    case TOKEN_DOT:
    {
        lexer->next_token();
        Ast_Expression *field = parse_expression();
        Ast_Field_Expression *field_expression = make_field_expression(operand, field);
        return field_expression;
    }
    case TOKEN_LPAREN:
    {
        lexer->next_token();
        Ast_Call_Expression *call_expression = make_call_expression(operand);
        while (!lexer->match_token(TOKEN_RPAREN)) {
            if (lexer->is_token(TOKEN_SEMICOLON)) {
                error("missing ')' before ';'");
            }
            Ast_Expression *argument = parse_expression();
            call_expression->arguments.push(argument);
        }
        return call_expression;
    }
    // case TOKEN_ASSIGN:
    }
}

Ast_Expression *Parser::parse_unary_expression() {
    if (is_unary_operator(lexer->token.type)) {
        Token_Type op = lexer->token.type;
        lexer->next_token();
        Ast_Unary_Expression *expression = make_unary_expression(op);
        expression->expression = parse_unary_expression();
        expression->op = op;
        return expression;
    } else {
        Ast_Expression *expression = parse_primary_expression();
        return expression;
    }
}

// 10 + 3 * 2
// 10 + (3 * 2)
//     +
//    /  \
//   /     *
//  10    / \
//       3   2
//

// 10 * 3 + 2
// (10 * 3) + 2
//     *
//    /  \
//   /     +
//  10    / \
//       3   2

// 10 * 3 + 2 * 4
// (10 * 3) + (2 * 4)
//        +
//       / \
//      /   \
//     /     \
//    *      * 
//   / \    / \
//  10  3  2   4

int precedence_table(Token_Type op) {
    switch (op) {
    default:
        return -1;
    case TOKEN_STAR: case TOKEN_SLASH: case TOKEN_PERCENT:
        return 500;
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return 400;
    case TOKEN_LT: case TOKEN_LTEQ: case TOKEN_GT: case TOKEN_GTEQ:
        return 300;
    case TOKEN_ASSIGN: case TOKEN_ADD_ASSIGN: case TOKEN_SUB_ASSIGN: case TOKEN_MUL_ASSIGN: case TOKEN_DIV_ASSIGN: case TOKEN_MOD_ASSIGN: case TOKEN_AND_ASSIGN: case TOKEN_XOR_ASSIGN: case TOKEN_OR_ASSIGN:
        return 100;
    case TOKEN_EQUAL: case TOKEN_NEQ:
        return 20;
    }
}

Ast_Expression *Parser::parse_binary_expression(Ast_Expression *lhs, int precedence) {
    for (;;) {
        int op_precedence = precedence_table(lexer->token.type);
        // leaf expression
        if (op_precedence < precedence) {
            return lhs;
        }

        Token_Type op = lexer->token.type;
        lexer->next_token();

        Ast_Expression *rhs = parse_unary_expression();
        if (!rhs) {
            return nullptr;
        }

        int next_precedence = precedence_table(lexer->token.type);
        if (op_precedence < next_precedence) {
            rhs = parse_binary_expression(rhs, op_precedence + 1);
            if (!rhs) return nullptr;
        }

        lhs = make_binary_expression(op, lhs, rhs);
    }
}

Ast_Expression *Parser::parse_expression() {
    Ast_Expression *expression = parse_unary_expression();
    expression = parse_binary_expression(expression, 0);
    
    return expression;
}

Ast_Statement *Parser::parse_init_statement(Ast_Expression *lhs) {
    if (lhs->type != AST_IDENT) {
        error("variable initialization must be preceded by an ident");
        return nullptr;
    }
    Ast_Variable *variable = parse_variable_declaration(static_cast<Ast_Ident *>(lhs));
    Ast_Declaration_Statement *declaration_statement = make_declaration_statement(variable);
    expect(TOKEN_SEMICOLON);
    return declaration_statement;
}

Ast_Statement *Parser::parse_simple_statement() {
    Ast_Expression *lhs = parse_expression();
    if (!lhs) return nullptr;
    
    switch (lexer->token.type) {
    default:
    {
        Ast_Expression_Statement *expression_statement = make_expression_statement(lhs);
        expect(TOKEN_SEMICOLON);
        return expression_statement;
    }
    case TOKEN_COLON:
    case TOKEN_COLON_ASSIGN: 
    {
        Ast_Statement *statement = parse_init_statement(lhs);
        return statement;
    }
    }
}

Ast_If *Parser::parse_if_statement() {
    assert(lexer->match_token(TOKEN_IF));
    Ast_Expression *condition = parse_expression();
    Ast_Block *block = parse_block();
    Ast_If *if_statement = make_if_statement(condition, block);
    return if_statement;
}

Ast_While *Parser::parse_while_statement() {
    assert(lexer->match_token(TOKEN_WHILE));
    Ast_Expression *condition = parse_expression();
    Ast_Block *block = parse_block();
    Ast_While *while_statement = make_while_statement(condition, block);
    return while_statement;
}

Ast_Return *Parser::parse_return_statement() {
    assert(lexer->match_token(TOKEN_RETURN));
    Ast_Expression *expression = parse_expression();
    Ast_Return *return_statement = make_return_statement(expression);
    expect(TOKEN_SEMICOLON);
    return return_statement;
}

Ast_Statement *Parser::parse_statement() {
    Ast_Statement *statement = nullptr;
    switch (lexer->token.type) {
    default:
    {
        statement = parse_simple_statement();
        break;
    }
    case TOKEN_SEMICOLON:
        lexer->next_token();
        break;
    case TOKEN_IF:
        statement = parse_if_statement();
        break;
    case TOKEN_WHILE:
        statement = parse_while_statement();
        break;
    case TOKEN_RETURN:
        statement = parse_return_statement();
        break;
    case TOKEN_ELSE:
    case TOKEN_DO:
    case TOKEN_FOR:
    case TOKEN_CASE:
    case TOKEN_CONTINUE:
    case TOKEN_BREAK:
        break;
    }
    return statement;
}

Ast_Block *Parser::parse_block() {
    Ast_Block *block = make_block();
    if (expect(TOKEN_LBRACE)) {
        do {
            if (lexer->is_token(TOKEN_EOF)) {
                error("unexpected end of file in block");
                break; 
            }

            Ast_Statement *statement = parse_statement();
            if (statement) block->statements.push(statement);
        } while (!lexer->match_token(TOKEN_RBRACE));
    }
    return block;
}

Ast_Procedure_Declaration *Parser::parse_procedure_declaration(Ast_Ident *ident) {
    Ast_Procedure_Declaration *procedure = make_procedure_declaration(ident);
    assert(lexer->match_token(TOKEN_COLON2));
    assert(lexer->match_token(TOKEN_LPAREN));

    if (lexer->is_token(TOKEN_IDENT)) {
        Ast_Ident *ident = make_ident(lexer->token.name);
        lexer->next_token();
        Ast_Variable *variable = parse_variable_declaration(ident);
        procedure->parameters.push(variable);

        while (lexer->match_token(TOKEN_COMMA)) {
            if (lexer->is_token(TOKEN_IDENT)) {
                ident = make_ident(lexer->token.name);
                lexer->next_token();
                variable = parse_variable_declaration(ident);
                procedure->parameters.push(variable);
            }
        }
    }
    expect(TOKEN_RPAREN);

    if (lexer->match_token(TOKEN_ARROW)) {
        procedure->return_type = parse_type_definition();
    }
    procedure->body = parse_block();

    return procedure;
}


Ast_Type_Definition *Parser::parse_type_definition() {
    Ast_Type_Definition *type_definition = nullptr;
    for (;;) {
        switch (lexer->token.type) {
        default:
            return type_definition;
        case TOKEN_IDENT:
            type_definition = make_type_definition(type_definition);
            type_definition->ident = make_ident(lexer->token.name);
            type_definition->type_flags = TYPE_DEFINITION_IDENT;
            lexer->next_token();
            break;
        case TOKEN_STAR:
            lexer->next_token();
            type_definition = make_type_definition(type_definition);
            type_definition->type_flags = TYPE_DEFINITION_POINTER;
            break;
        case TOKEN_LBRACKET:
            lexer->next_token();
            expect(TOKEN_RBRACKET);
            type_definition = make_type_definition(type_definition);
            break;
        }
    }
    return type_definition;
}

Ast_Variable *Parser::parse_variable_declaration(Ast_Ident *ident) {
    Ast_Variable *variable = make_variable_declaration(ident);
    if (lexer->match_token(TOKEN_COLON)) {
        Ast_Type_Definition *type_definition = parse_type_definition();
        variable->type_definition = type_definition;
        if (lexer->match_token(TOKEN_ASSIGN)) {
            Ast_Expression *rhs = parse_expression();
            variable->initializer = rhs;
        }
    } else if (lexer->match_token(TOKEN_COLON_ASSIGN)) {
        Ast_Expression *expression = parse_expression();
        variable->initializer = expression;
    }
    return variable;
}

Ast_Struct_Declaration *Parser::parse_struct_declaration(Ast_Ident *ident) {
    assert(lexer->match_token(TOKEN_COLON2));
    assert(lexer->match_token(TOKEN_STRUCT));

    expect(TOKEN_LBRACE);

    Ast_Struct_Declaration *struct_declaration = make_struct_declaration(ident);
    while (lexer->is_token(TOKEN_IDENT)) {
        Ast_Ident *ident = make_ident(lexer->token.name);
        lexer->next_token();
        expect(TOKEN_COLON);
        Ast_Type_Definition *type_definition = parse_type_definition();
        if (!expect(TOKEN_SEMICOLON)) {
            break;
        }
        Ast_Struct_Field *field = make_struct_field(ident, type_definition);
        struct_declaration->fields.push(field);
    }
    expect(TOKEN_RBRACE);
    return struct_declaration;
}

Ast_Declaration *Parser::parse_declaration() {
    Ast_Ident *ident = nullptr;
    if (lexer->is_token(TOKEN_IDENT)) {
        Atom *name = lexer->token.name;
        lexer->next_token();
        ident = make_ident(name);
    } else {
        return nullptr;
    }

    if (lexer->is_token(TOKEN_COLON)) {
        Ast_Variable *variable = parse_variable_declaration(ident);
        expect(TOKEN_SEMICOLON);
        return variable;
    } else if (lexer->is_token(TOKEN_COLON2)) {
        Token token = lexer->peek_token();
        switch (token.type) {
        case TOKEN_LPAREN:
        {
            Ast_Procedure_Declaration *procedure = parse_procedure_declaration(ident);
            return procedure;
        }
        case TOKEN_STRUCT:
        {
            Ast_Struct_Declaration *struct_declaration = parse_struct_declaration(ident);
            return struct_declaration;
        }
        case TOKEN_ENUM:
            break;
        }
    }
    return nullptr;
}

Ast_Root *Parser::parse_root() {
    Ast_Root *root = make_root();
    for (;;) {
        if (lexer->is_token(TOKEN_EOF))
            break;
        
        Ast_Declaration *declaration = parse_declaration();
        if (declaration) {
            root->declarations.push(declaration);
        }
    }
    return root;
}
