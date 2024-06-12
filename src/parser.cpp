#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "core.h"
#include "parser.h"

Arena ast_arena = make_arena();

char *type_to_string(Ast_Type_Info *type) {
    static char buffer[1024];
    memset(buffer, 0, 1024);
    
    for (Ast_Type_Info *it = type; it; it = it->base) {
        switch (it->type_kind) {
        default:
            assert(0);
            break;
        case TypeKind_Pointer:
            strcat(buffer, "*");
            break;
        case TypeKind_Array:
            strcat(buffer, "[]");
            break;
        case TypeKind_Float32:
            strcat(buffer, "float32");
            break;
        case TypeKind_Float64:
            strcat(buffer, "float64");
            break;
        case TypeKind_Int:
            strcat(buffer, "int");
            break;
        case TypeKind_Int8:
            strcat(buffer, "int8");
            break;
        case TypeKind_Int16:
            strcat(buffer, "int16");
            break;
        case TypeKind_Int32:
            strcat(buffer, "int32");
            break;
        case TypeKind_Int64:
            strcat(buffer, "int64");
            break;
        case TypeKind_Uint:
            strcat(buffer, "uint");
            break;
        case TypeKind_Uint8:
            strcat(buffer, "uint8");
            break;
        case TypeKind_Uint16:
            strcat(buffer, "uint16");
            break;
        case TypeKind_Uint32:
            strcat(buffer, "uint32");
            break;
        case TypeKind_Uint64:
            strcat(buffer, "uint64");
            break;
        }
    }

    size_t len = strlen(buffer);
    char *str = (char *)malloc(len + 1);
    memcpy(str, buffer, len);
    str[len] = 0;
    return str;
}

char *type_definition_to_string(Ast_Type_Definition *type_definition) {
    static char buffer[1024];
    memset(buffer, 0, 1024);
    if (!type_definition) strcat(buffer, "<auto>");
    for (Ast_Type_Definition *it = type_definition; it; it = it->base) {
        if (it->defn_flags & TypeDefnFlag_Pointer) strcat(buffer, "*");
        else if (it->defn_flags & TypeDefnFlag_Array) strcat(buffer, "[]");
        else if (it->defn_flags & TypeDefnFlag_Ident) strcat(buffer, it->ident->name->name);
    }

    size_t len = strlen(buffer);
    char *str = (char *)malloc(len + 1);
    memcpy(str, buffer, len);
    str[len] = 0;
    return str;
}

void Parser::error(const char *fmt, ...) {
    printf("%s(%d,%d) error: ", lexer->source_name, lexer->l0, lexer->c0);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    //@todo error handling
    // while (!lexer->is_token(Token_Semicolon) && !lexer->is_token(Token_CloseBrace)) lexer->next_token();
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
    Ast *result = (Ast *)arena_alloc(&ast_arena, size);
    return result;
}
#define AST_NEW(AST_T) (AST_T *)(&(*allocate_ast_node(sizeof(AST_T)) = AST_T()))

Ast_Declaration *make_declaration(Ast_Ident *ident) {
    Ast_Declaration *declaration = AST_NEW(Ast_Declaration);
    declaration->ident = ident;
    return declaration;
}

static inline Ast_Root *make_root() {
    Ast_Root *root = AST_NEW(Ast_Root);
    return root;
}

Ast_Ident *make_ident(Atom *name) {
    Ast_Ident *ident = AST_NEW(Ast_Ident);
    ident->name = name;
    return ident;
}

Ast_Type_Info *make_type_info(Type_Kind kind, Type_Info_Flags flags, int bits, Ast_Type_Info *base) {
    Ast_Type_Info *type_info = AST_NEW(Ast_Type_Info);
    type_info->type_kind = kind;
    type_info->type_flags |= flags;
    type_info->bits = bits;
    type_info->base = base;
    return type_info;
}

static inline Ast_Variable *make_variable_declaration(Ast_Ident *ident) {
    Ast_Variable *variable = AST_NEW(Ast_Variable);
    variable->ident = ident;
    return variable;
}

static inline Ast_Procedure_Declaration *make_procedure_declaration(Ast_Ident *ident) {
    Ast_Procedure_Declaration *procedure = AST_NEW(Ast_Procedure_Declaration);
    procedure->ident = ident;
    procedure->declaration_flags |= DeclFlag_Global;
    return procedure;
}

static inline Ast_Struct_Declaration *make_struct_declaration(Ast_Ident *ident) {
    Ast_Struct_Declaration *declaration = AST_NEW(Ast_Struct_Declaration);
    declaration->ident = ident;
    declaration->declaration_flags |= DeclFlag_Global;
    return declaration;
}

static inline Ast_Struct_Field *make_struct_field(Atom *name, Ast_Type_Definition *type_definition) {
    Ast_Struct_Field *field = AST_NEW(Ast_Struct_Field);
    field->name = name;
    field->type_definition = type_definition;
    return field;
}

static inline Ast_Enum_Declaration *make_enum_declaration(Ast_Ident *ident) {
    Ast_Enum_Declaration *declaration = AST_NEW(Ast_Enum_Declaration);
    declaration->ident = ident;
    return declaration;
}

static inline Ast_Enum_Field *make_enum_field(Atom *name, int64 value) {
    Ast_Enum_Field *field = AST_NEW(Ast_Enum_Field);
    field->name = name;
    field->value = value;
    return field;
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

static inline Ast_Block_Statement *make_block_statement(Ast_Block *block) {
    Ast_Block_Statement *block_statement = AST_NEW(Ast_Block_Statement);
    block_statement->block = block;
    return block_statement;
}

static inline Ast_Block *make_block() {
    Ast_Block *block = AST_NEW(Ast_Block);
    return block;
}

static inline Ast_Literal *make_integer_literal(uint64 value) {
    Ast_Literal *literal = AST_NEW(Ast_Literal);
    literal->literal_flags |= LiteralFlag_Number;
    literal->int_value = value;
    return literal;
}

static inline Ast_Literal *make_float_literal(float64 value) {
    Ast_Literal *literal = AST_NEW(Ast_Literal);
    literal->literal_flags |= LiteralFlag_Number;
    literal->literal_flags |= LiteralFlag_Float;
    literal->float_value = value;
    return literal;
}

static inline Ast_Literal *make_string_literal(char *value) {
    Ast_Literal *literal = AST_NEW(Ast_Literal);
    literal->literal_flags |= LiteralFlag_String;
    literal->string_value = value;
    return literal;
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

static inline Ast_Range_Expression *make_range_expression(Ast_Expression *first, Ast_Expression *last) {
    Ast_Range_Expression *range_expression = AST_NEW(Ast_Range_Expression);
    range_expression->first = first;
    range_expression->last = last;
    return range_expression;
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

static inline Ast_For *make_for_statement(Ast_Ident *iterator, Ast_Range_Expression *range, Ast_Block *block) {
    Ast_For *for_statement = AST_NEW(Ast_For);
    for_statement->iterator = iterator;
    for_statement->range = range;
    for_statement->block = block;
    return for_statement;
}

static inline Ast_Break *make_break_statement() {
    Ast_Break *break_statement = AST_NEW(Ast_Break);
    return break_statement;
}

static inline Ast_Return *make_return_statement(Ast_Expression *expression) {
    Ast_Return *return_statement = AST_NEW(Ast_Return);
    return_statement->expression = expression;
    return return_statement;
}

Ast_Expression *Parser::parse_operand() {
    Ast_Expression *expression = nullptr;
    Source_Loc start = get_start_loc();
    switch (lexer->token.type) {
    case Token_Ident:
    {
        Ast_Ident *ident = make_ident(lexer->token.name);
        ident->expr_flags |= ExprFlag_Lvalue;
        lexer->next_token();
        expression = ident;
        break;
    }
    case Token_Intlit: 
    {
        Ast_Literal *literal = make_integer_literal(lexer->token.intlit);
        lexer->next_token();
        expression = literal;
        break;
    }
    case Token_Floatlit:
    {
        Ast_Literal *literal = make_float_literal(lexer->token.floatlit);
        lexer->next_token();
        expression = literal;
        break;
    }
    case Token_Strlit:
    {
        Ast_Literal *literal = make_string_literal(lexer->token.strlit);
        lexer->next_token();
        expression = literal;
        break;
    }
    }

    if (expression) {
        expression->start = start;
        end_loc(expression);
    }
    return expression;
}

Ast_Expression *Parser::parse_primary_expression() {
    Source_Loc start = get_start_loc();
    Ast_Expression *operand = parse_operand();
    Ast_Expression *primary = operand;
    switch (lexer->token.type) {
    case Token_OpenBracket:
    {
        lexer->next_token();
        Ast_Expression *index = parse_expression();
        Ast_Index_Expression *index_expression = make_index_expression(operand, index);
        expect(Token_CloseBracket);
        primary = index_expression;
        break;
    }
    case Token_Dot:
    {
        lexer->next_token();
        Ast_Expression *field = parse_expression();
        Ast_Field_Expression *field_expression = make_field_expression(operand, field);
        primary = field_expression;
        break;
    }
    case Token_OpenParen:
    {
        lexer->next_token();
        Ast_Call_Expression *call_expression = make_call_expression(operand);
        do {
            Ast_Expression *argument = parse_expression();
            if (!argument) break;
            call_expression->arguments.push(argument);
        } while (lexer->match_token(Token_Comma));
        expect(Token_CloseParen);
        primary = call_expression;
        break;
    }
    }

    if (primary) {
        primary->start = start;
        end_loc(primary);
    }
    return primary;
}

Ast_Expression *Parser::parse_unary_expression() {
    Ast_Expression *expression = nullptr;
    Source_Loc start = get_start_loc();
    if (is_unary_operator(lexer->token.type)) {
        Token_Type op = lexer->token.type;
        lexer->next_token();
        Ast_Unary_Expression *unary = make_unary_expression(op);
        unary->expression = parse_unary_expression();
        unary->op = op;
        expression = unary;
    } else {
        Ast_Expression *primary = parse_primary_expression();
        expression = primary;
    }
    if (expression) {
        expression->start = start;
        end_loc(expression);
    }
    return expression;
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
    case Token_Star: case Token_Slash: case Token_Percent:
        return 500;
    case Token_Plus:
    case Token_Minus:
        return 400;
    case Token_Lt: case Token_Lteq: case Token_Gt: case Token_Gteq:
        return 300;
    case Token_Assign: case Token_AddAssign: case Token_SubAssign: case Token_MulAssign: case Token_DivAssign: case Token_ModAssign: case Token_AndAssign: case Token_XorAssign: case Token_OrAssign:
        return 100;
    case Token_Equal: case Token_Neq:
        return 20;
    }
}

Ast_Expression *Parser::parse_binary_expression(Ast_Expression *lhs, int precedence) {
    for (;;) {
        Token op = lexer->token;
        int op_precedence = precedence_table(op.type);
        // leaf expression
        if (op_precedence < precedence) {
            return lhs;
        }

        lexer->next_token();

        Ast_Expression *rhs = parse_unary_expression();
        if (!rhs) {
            return nullptr;
        }

        int next_precedence = precedence_table(lexer->token.type);
        if (op_precedence < next_precedence) {
            rhs = parse_binary_expression(rhs, op_precedence + 1);
            if (rhs) {
            } else {
                return nullptr;
            }
        }

        lhs = make_binary_expression(op.type, lhs, rhs);
        lhs->start = op.start;
    }
}

Ast_Expression *Parser::parse_expression() {
    Ast_Expression *expression = parse_unary_expression();
    expression = parse_binary_expression(expression, 0);
    return expression;
}

Ast_Statement *Parser::parse_init_statement(Ast_Expression *lhs) {
    assert(lhs);
    if (lhs->kind != AstKind_Ident) {
        error("variable initialization must be begin with an identifier");
        return nullptr;
    }
    Ast_Variable *variable = parse_variable_declaration(static_cast<Ast_Ident *>(lhs));
    Ast_Declaration_Statement *declaration_statement = make_declaration_statement(variable);
    expect(Token_Semicolon);
    return declaration_statement;
}

Ast_Statement *Parser::parse_simple_statement() {
    Ast_Expression *lhs = parse_expression();
    if (!lhs) return nullptr;

    Ast_Statement *statement = nullptr;
    switch (lexer->token.type) {
    default:
    {
        Ast_Expression_Statement *expression_statement = make_expression_statement(lhs);
        expect(Token_Semicolon);
        statement = expression_statement;
        break;
    }
    case Token_Colon:
    case Token_ColonAssign:
    {
        statement = parse_init_statement(lhs);
        break;
    }
    }
    statement->start = lhs->start;
    end_loc(statement);
    return statement;
}

Ast_If *Parser::parse_if_statement() {
    assert(lexer->match_token(Token_If));
    Source_Loc start = get_start_loc();
    Ast_Expression *condition = parse_expression();
    Ast_Block *block = parse_block();
    Ast_If *if_statement = make_if_statement(condition, block);
    if_statement->start = start;
    end_loc(if_statement);

    Ast_If *tail = if_statement;
    while (lexer->match_token(Token_Else)) {
        bool last = false;
        Source_Loc start = get_start_loc();
        Ast_Expression *condition = nullptr;
        if (lexer->match_token(Token_If)) {
            condition = parse_expression();
        } else last = true;
        Ast_Block *block = parse_block();
        Ast_If *elif = make_if_statement(condition, block);
        tail->next = elif;
        tail = elif;
        elif->start = start;
        end_loc(elif);
        
        if (last) break;
    }
    return if_statement;
}

Ast_While *Parser::parse_while_statement() {
    assert(lexer->match_token(Token_While));
    Source_Loc start = get_start_loc();
    Ast_Expression *condition = parse_expression();
    Ast_Block *block = parse_block();
    Ast_While *while_statement = make_while_statement(condition, block);
    while_statement->start = start;
    end_loc(while_statement);
    return while_statement;
}

Ast_Range_Expression *Parser::parse_range_expression() {
    Ast_Expression *first = parse_expression();
    expect(Token_Ellipsis);
    Ast_Expression *last = parse_expression();
    assert(first && last);
    Ast_Range_Expression *expression = make_range_expression(first, last);
    return expression;
}

Ast_For *Parser::parse_for_statement() {
    assert(lexer->match_token(Token_For));
    Source_Loc start = get_start_loc();

    Ast_Ident *ident = parse_ident();
    expect(Token_Colon);
    Ast_Range_Expression *range_expression = nullptr;
    if (lexer->is_token(Token_Ident)) {
        //@todo iterable type like Array
        Ast_Ident *iteratable = parse_ident();
    } else {
        range_expression = parse_range_expression();
    }

    Ast_Block *block = parse_block();

    Ast_For *for_statement = make_for_statement(ident, range_expression, block);
    for_statement->start = start;
    end_loc(for_statement);
    return for_statement;
}

Ast_Break *Parser::parse_break_statement() {
    assert(expect(Token_Break));
    Ast_Break *break_statement = make_break_statement();
    expect(Token_Semicolon);
    return break_statement; 
}

Ast_Return *Parser::parse_return_statement() {
    assert(expect(Token_Return));
    Source_Loc start = get_start_loc();
    Ast_Expression *expression = parse_expression();
    Ast_Return *return_statement = make_return_statement(expression);
    expect(Token_Semicolon);
    return_statement->start = start;
    end_loc(return_statement);
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
    case Token_OpenBrace:
    {
        Ast_Block *block = parse_block();
        statement = make_block_statement(block);
        break;
    }
    case Token_Semicolon:
        lexer->next_token();
        break;
    case Token_If:
        statement = parse_if_statement();
        break;
    case Token_Else:
        error("illegal else without matching if");
        lexer->next_token();
        break;
    case Token_While:
        statement = parse_while_statement();
        break;
    case Token_For:
        statement = parse_for_statement();
        break;
    case Token_Break:
        statement = parse_break_statement();
        break;
    case Token_Return:
        statement = parse_return_statement();
        break;
    case Token_Do:
    case Token_Case:
    case Token_Continue:
        break;
    }
    return statement;
}

Ast_Block *Parser::parse_block() {
    assert(expect(Token_OpenBrace));
    Ast_Block *block = make_block();
    start_loc(block);
    do {
        if (lexer->is_token(Token_EOF)) {
            error("unexpected end of file in block");
            break;
        }

        Ast_Statement *statement = parse_statement();
        if (statement) block->statements.push(statement);
    } while (!lexer->match_token(Token_CloseBrace));
    end_loc(block);
    return block;
}

Ast_Procedure_Declaration *Parser::parse_procedure_declaration(Ast_Ident *ident) {
    assert(lexer->match_token(Token_Colon2));
    assert(lexer->match_token(Token_OpenParen));

    Ast_Procedure_Declaration *procedure = make_procedure_declaration(ident);

    if (lexer->is_token(Token_Ident)) {
        Ast_Ident *ident = make_ident(lexer->token.name);
        lexer->next_token();
        Ast_Variable *variable = parse_variable_declaration(ident);
        procedure->parameters.push(variable);

        while (lexer->match_token(Token_Comma)) {
            if (lexer->is_token(Token_Ident)) {
                ident = make_ident(lexer->token.name);
                lexer->next_token();
                variable = parse_variable_declaration(ident);
                procedure->parameters.push(variable);
            }
        }
    }
    expect(Token_CloseParen);

    if (lexer->match_token(Token_Arrow)) {
        procedure->return_type = parse_type_definition();
    }
    procedure->body = parse_block();
    return procedure;
}

Ast_Type_Definition *Parser::parse_type_definition() {
    Source_Loc start = get_start_loc();
    Ast_Type_Definition *type_definition = nullptr;
    for (;;) {
        switch (lexer->token.type) {
        default:
            goto end_defn;
        case Token_Ident:
            type_definition = make_type_definition(type_definition);
            type_definition->ident = make_ident(lexer->token.name);
            type_definition->defn_flags |= TypeDefnFlag_Ident;
            lexer->next_token();
            break;
        case Token_Star:
            lexer->next_token();
            type_definition = make_type_definition(type_definition);
            type_definition->defn_flags |= TypeDefnFlag_Pointer;
            break;
        case Token_OpenBracket:
            lexer->next_token();
            type_definition = make_type_definition(type_definition);
            if (lexer->match_token(Token_Ellipsis)) {
                type_definition->defn_flags |= TypeDefnFlag_DynamicArray;
            } else {
                type_definition->defn_flags |= TypeDefnFlag_Array;
                type_definition->array_size = parse_expression();
            }
            expect(Token_CloseBracket);
            break;
        }
    }
end_defn:
    if (type_definition) {
        type_definition->start = start;
        end_loc(type_definition);
    }
    return type_definition;
}

Ast_Variable *Parser::parse_variable_declaration(Ast_Ident *ident) {
    Ast_Variable *variable = make_variable_declaration(ident);
    if (lexer->match_token(Token_Colon)) {
        Ast_Type_Definition *type_definition = parse_type_definition();
        variable->type_definition = type_definition;
        if (lexer->match_token(Token_Assign)) {
            Ast_Expression *rhs = parse_expression();
            variable->initializer = rhs;
        }
    } else if (lexer->match_token(Token_ColonAssign)) {
        Ast_Expression *expression = parse_expression();
        variable->initializer = expression;
    }
    return variable;
}

Ast_Struct_Declaration *Parser::parse_struct_declaration(Ast_Ident *ident) {
    assert(lexer->match_token(Token_Colon2));
    assert(lexer->match_token(Token_Struct));
    expect(Token_OpenBrace);

    Ast_Struct_Declaration *struct_declaration = make_struct_declaration(ident);
    while (lexer->is_token(Token_Ident)) {
        Source_Loc start = get_start_loc();
        Token name = lexer->token;
        lexer->next_token();
        expect(Token_Colon);
        Ast_Type_Definition *type_definition = parse_type_definition();
        Ast_Struct_Field *field = make_struct_field(name.name, type_definition);
        struct_declaration->fields.push(field);
        field->start = start;
        end_loc(field);
        if (!expect(Token_Semicolon)) {
            break;
        }
    }
    expect(Token_CloseBrace);
    return struct_declaration;
}

Ast_Enum_Declaration *Parser::parse_enum_declaration(Ast_Ident *ident) {
    assert(expect(Token_Colon2));
    assert(expect(Token_Enum));
    assert(expect(Token_OpenBrace));

    int64 field_value = 0;
    Ast_Enum_Declaration *enum_declaration = make_enum_declaration(ident);
    do {
        if (!lexer->is_token(Token_Ident)) break;

        Atom *name = lexer->token.name;
        lexer->next_token();
        Ast_Enum_Field *field = make_enum_field(name, field_value++);
        enum_declaration->fields.push(field);
        field->start = ident->start;
        end_loc(field);
    } while (lexer->match_token(Token_Comma));

    expect(Token_CloseBrace);
    return enum_declaration;
}

Ast_Ident *Parser::parse_ident() {
    assert(lexer->is_token(Token_Ident));
    Token name = lexer->token;
    Ast_Ident *ident = make_ident(name.name);
    start_loc(ident);
    end_loc(ident);
    lexer->next_token();
    return ident;
}

Ast_Declaration *Parser::parse_declaration() {
    Ast_Ident *ident = nullptr;
    if (lexer->is_token(Token_Ident)) {
        ident = parse_ident(); 
    } else {
        return nullptr;
    }

    Ast_Declaration *declaration = nullptr;
    if (lexer->is_token(Token_Colon)) {
        Ast_Variable *variable = parse_variable_declaration(ident);
        variable->declaration_flags |= DeclFlag_Global;
        expect(Token_Semicolon);
        declaration = variable;
    } else if (lexer->is_token(Token_Colon2)) {
        Token token = lexer->peek_token();
        switch (token.type) {
        case Token_OpenParen:
        {
            Ast_Procedure_Declaration *procedure = parse_procedure_declaration(ident);
            declaration = procedure;
            break;
        }
        case Token_Struct:
        {
            Ast_Struct_Declaration *struct_declaration = parse_struct_declaration(ident);
            declaration = struct_declaration;
            break;
        }
        case Token_Enum:
            Ast_Enum_Declaration *enum_declaration = parse_enum_declaration(ident);
            break;
        }
    } else {
        error("expected ':' or '::' for declaration, got '%s'", token_type_to_string(lexer->token.type));
    }
    
    if (declaration) {
        declaration->start = ident->start;
        end_loc(declaration);
    }
    return declaration;
}

Ast_Root *Parser::parse_root() {
    Ast_Root *root = make_root();
    for (;;) {
        if (lexer->is_token(Token_EOF))
            break;

        Ast_Declaration *declaration = parse_declaration();
        if (declaration) {
            root->declarations.push(declaration);
        }
    }
    return root;
}
