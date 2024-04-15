#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "parser.h"

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
    error("expected '%s', got '%s'", token_type_to_string(type));
    return false;
}

inline Ast *allocate_ast_node(size_t size) {
    Ast *result = (Ast *)calloc(size, 1);
    return result;
}
#define AST_NEW(AST_T) (AST_T *)(&(*allocate_ast_node(sizeof(AST_T)) = AST_T()))

inline Ast_Ident *make_ident(char *name) {
    Ast_Ident *ident = AST_NEW(Ast_Ident);
    ident->name =  name;
    return ident;
}

inline Ast_Procedure_Declaration *make_procedure_declaration(Ast_Ident *ident) {
    Ast_Procedure_Declaration *procedure = AST_NEW(Ast_Procedure_Declaration);
    procedure->ident = ident;
    return procedure;
}

inline Ast_Declaration_Statement *make_declaration_statement(Ast_Declaration *declaration) {
    Ast_Declaration_Statement *declaration_statement = AST_NEW(Ast_Declaration_Statement);
    declaration_statement->declaration = declaration;
    return declaration_statement;
}

inline Ast_Expression_Statement *make_expression_statement(Ast_Expression *expression) {
    Ast_Expression_Statement *expression_statement = AST_NEW(Ast_Expression_Statement);
    expression_statement->expression = expression;
    return expression_statement;
}

inline Ast_Block *make_block() {
    Ast_Block *block = AST_NEW(Ast_Block);
    return block;
}

inline Ast_Literal *make_integer_literal(int64 value) {
    Ast_Literal *literal = AST_NEW(Ast_Literal);
    literal->literal_flags |= LITERAL_NUMBER;
    literal->int_value = value;
    return literal;
}

inline Ast_Literal *make_float_literal(float64 value) {
    Ast_Literal *literal = AST_NEW(Ast_Literal);
    literal->literal_flags |= LITERAL_NUMBER;
    literal->literal_flags |= LITERAL_FLOAT;
    literal->float_value = value;
    return literal;
}

inline Ast_Literal *make_string_literal(char *value) {
    Ast_Literal *literal = AST_NEW(Ast_Literal);
    literal->literal_flags |= LITERAL_STRING;
    literal->string_value = value;
    return literal;
}

inline Ast_Variable *make_variable_declaration(Ast_Ident *ident) {
    Ast_Variable *variable = AST_NEW(Ast_Variable);
    variable->ident = ident;
    return variable;
}

inline Ast_Type_Definition *make_type_definition(Ast_Type_Definition *base) {
    Ast_Type_Definition *type_definition = AST_NEW(Ast_Type_Definition);
    type_definition->base = base;
    return type_definition;
}

inline Ast_Unary_Expression *make_unary_expression(Token_Type op) {
    Ast_Unary_Expression *unary = AST_NEW(Ast_Unary_Expression);
    unary->op = op;
    return unary;
}

inline Ast_Index_Expression *make_index_expression(Ast_Expression *array, Ast_Expression *index) {
    Ast_Index_Expression *index_expression = AST_NEW(Ast_Index_Expression);
    index_expression->array = array;
    index_expression->index = index;
    return index_expression;
}

inline Ast_Field_Expression *make_field_expression(Ast_Expression *operand, Ast_Expression *field) {
    Ast_Field_Expression *field_expression = AST_NEW(Ast_Field_Expression);
    field_expression->operand = operand;
    field_expression->field = field;
    return field_expression;
}

inline Ast_Call_Expression *make_call_expression(Ast_Expression *operand) {
    Ast_Call_Expression *call_expression = AST_NEW(Ast_Call_Expression);
    call_expression->operand = operand;
    return call_expression;
}

Ast_Expression *Parser::parse_operand() {
    switch (lexer->token.type) {
    case TOKEN_IDENT:
    {
        Ast_Ident *ident = make_ident(lexer->token.strlit);
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
    case TOKEN_LBRACE:
    {
        lexer->next_token();
        Ast_Expression *index = parse_expression();
        Ast_Index_Expression *index_expression = make_index_expression(operand, index);
        expect(TOKEN_RBRACE);
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
                error("';' before ')'");
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

Ast_Expression *Parser::parse_expression() {
    Ast_Expression *expression = parse_unary_expression();
    return expression;
}

Ast_Statement *Parser::parse_init_statement(Ast_Expression *lhs) {
    if (lhs->type != AST_IDENT) {
        error("variable initialization must be preceded by an ident");
        return nullptr;
    }
    Ast_Variable *variable = parse_variable_declaration(static_cast<Ast_Ident *>(lhs));
    Ast_Declaration_Statement *declaration_statement = make_declaration_statement(variable);
    return declaration_statement;
}

Ast_Statement *Parser::parse_simple_statement() {
    Ast_Expression *lhs = parse_expression();
    switch (lexer->token.type) {
    default:
    {
        Ast_Expression_Statement *expression_statement = make_expression_statement(lhs);
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

Ast_Statement *Parser::parse_statement() {
    Ast_Statement *statement = nullptr;
    switch (lexer->token.type) {
    default:
    {
        statement = parse_simple_statement();
        break;
    }
    case TOKEN_IF:
    case TOKEN_ELSE:
    case TOKEN_WHILE:
    case TOKEN_DO:
    case TOKEN_FOR:
    case TOKEN_CASE:
    case TOKEN_CONTINUE:
    case TOKEN_BREAK:
    case TOKEN_RETURN:
        break;
    }
    expect(TOKEN_SEMICOLON);
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
        } while (!lexer->match_token(TOKEN_RBRACE));
    }
    return block;
}

Ast_Procedure_Declaration *Parser::parse_procedure_declaration(Ast_Ident *ident) {
    Ast_Procedure_Declaration *procedure = make_procedure_declaration(ident);
    assert(lexer->match_token(TOKEN_COLON2));

    if (expect(TOKEN_LPAREN)) {
        // @todo Parse parameters
        expect(TOKEN_RPAREN);

        if (lexer->match_token(TOKEN_ARROW)) {
            procedure->return_type = parse_type_definition();
        }

        procedure->body = parse_block();
    }

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
            type_definition->ident = make_ident(lexer->token.strlit);
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
            expect(TOKEN_RBRACE);
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

Ast_Declaration *Parser::parse_declaration() {
    if (lexer->is_token(TOKEN_IDENT)) {
        char *name = lexer->token.strlit;
        lexer->next_token();
        Ast_Ident *ident = make_ident(name);

        switch (lexer->token.type) {
        case TOKEN_COLON:
        {
            Ast_Variable *variable = parse_variable_declaration(ident);
            expect(TOKEN_SEMICOLON);
            return variable;
        }
        case TOKEN_COLON2:
        {
            Ast_Procedure_Declaration *procedure = parse_procedure_declaration(ident);
            return procedure;
        }
        }
    }
    return nullptr;
}

inline Ast_Root *make_root() {
    Ast_Root *root = AST_NEW(Ast_Root);
    return root;
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
 
