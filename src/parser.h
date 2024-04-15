#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "array.h"

enum Ast_Type {
    AST_NONE,

    AST_ROOT,
    AST_TYPE_DEFINITION,
    AST_BLOCK,

    AST_DECLARATION,
    AST_PROCEDURE,
    AST_VARIABLE,
    AST_STRUCT,
    AST_UNION,
    AST_ENUM,

    AST_STATEMENT,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_DECLARATION_STATEMENT,
    AST_EXPRESSION_STATEMENT,

    AST_EXPRESSION,
    AST_BINARY_EXPRESSION,
    AST_UNARY_EXPRESSION,
    AST_INDEX_EXPRESSION,
    AST_FIELD_EXPRESSION,
    AST_CALL_EXPRESSION,
    // AST_ASSIGN_EXPRESSION,
    AST_LITERAL,
    AST_IDENT,
};

struct Ast_Statement;
struct Ast_Expression;
struct Ast_Declaration;
struct Ast_Ident;
struct Ast_Block;

struct Ast {
    Ast_Type type = AST_NONE;
};

struct Ast_Expression : Ast {
};

struct Ast_Declaration : Ast {
    Ast_Ident *ident;
};

struct Ast_Statement : Ast {
};

struct Ast_Root : Ast {
    Ast_Root() { type = AST_ROOT; }
    Array<Ast_Declaration *> declarations;
};

constexpr int TYPE_DEFINITION_POINTER = 0x1;
constexpr int TYPE_DEFINITION_ARRAY = 0x2;
constexpr int TYPE_DEFINITION_IDENT = 0x4;

struct Ast_Type_Definition : Ast {
    Ast_Type_Definition() {type = AST_TYPE_DEFINITION;}
    int type_flags;
    Ast_Ident *ident;
    Ast_Type_Definition *base;
};

struct Ast_Block : Ast {
    Ast_Block() { type = AST_BLOCK; }
    Array<Ast_Statement *> statements;
};

struct Ast_Variable : Ast_Declaration {
    Ast_Variable() { type = AST_VARIABLE; }
    Ast_Type_Definition *type_definition;
    Ast_Expression *initializer;
};

struct Ast_Procedure_Declaration : Ast_Declaration {
    Ast_Procedure_Declaration() { type = AST_PROCEDURE; }
    Array<Ast_Variable *> parameters;
    Ast_Type_Definition *return_type;
    Ast_Block *body;
};

struct Ast_Declaration_Statement : Ast_Statement {
    Ast_Declaration_Statement() { type = AST_DECLARATION_STATEMENT; }
    Ast_Declaration *declaration;
};

struct Ast_Expression_Statement : Ast_Statement {
    Ast_Expression_Statement() { type = AST_EXPRESSION_STATEMENT; }
    Ast_Expression *expression;
};

struct Ast_If : Ast_Statement {
    Ast_Expression *condition;
    Ast_Block *block;
};

struct Ast_While : Ast_Statement {
    Ast_Expression *condition;
    Ast_Block *block;
};

struct Ast_Return : Ast_Statement {
    Ast_Expression *expression;
};

struct Ast_Binary_Expression : Ast_Expression {
    Ast_Binary_Expression() { type = AST_BINARY_EXPRESSION; }
    Token_Type op;
    Ast_Expression *lhs;
    Ast_Expression *rhs;
};

struct Ast_Unary_Expression : Ast_Expression {
    Ast_Unary_Expression() { type = AST_UNARY_EXPRESSION; }
    Token_Type op;
    Ast_Expression *expression;
};

struct Ast_Index_Expression : Ast_Expression {
    Ast_Index_Expression() { type = AST_INDEX_EXPRESSION; }
    Ast_Expression *array;
    Ast_Expression *index;
};

struct Ast_Call_Expression : Ast_Expression {
    Ast_Call_Expression() { type = AST_CALL_EXPRESSION; }
    Ast_Expression *operand;
    Array<Ast_Expression *> arguments;
};

struct Ast_Field_Expression : Ast_Expression {
    Ast_Field_Expression() { type = AST_FIELD_EXPRESSION; }
    Ast_Expression *operand;
    Ast_Expression *field;
};

constexpr int LITERAL_NUMBER = 0x1;
constexpr int LITERAL_STRING = 0x2;
constexpr int LITERAL_FLOAT = 0x4;

struct Ast_Literal : Ast_Expression {
    Ast_Literal() { type = AST_LITERAL; }
    int literal_flags;
    union {
        int64 int_value;
        float64 float_value;
        char *string_value;
    };
};

struct Ast_Ident : Ast_Expression {
    Ast_Ident() { type = AST_IDENT; }
    char *name;
};

struct Parser {
    Ast_Root *root = nullptr;
    Lexer *lexer = nullptr;

    Parser(char *file_name) {
        lexer = new Lexer(file_name);

        root = parse_root();
    }

    void error(const char *fmt, ...);
    bool expect(Token_Type type);

    Ast_Expression *parse_expression();
    Ast_Expression *parse_primary_expression();
    Ast_Expression *parse_unary_expression();
    Ast_Expression *parse_operand();
    // Ast_Expression *parse_binary_expression();
    Ast_Block *parse_block();
    Ast_Statement *parse_init_statement(Ast_Expression *lhs);
    Ast_Statement *parse_simple_statement();
    Ast_Statement *parse_statement();
    Ast_Type_Definition *parse_type_definition();
    Ast_Variable *parse_variable_declaration(Ast_Ident *identfier);
    Ast_Procedure_Declaration *parse_procedure_declaration(Ast_Ident *ident);
    Ast_Declaration *parse_declaration();
    Ast_Root *parse_root();
};

#endif // PARSER_H
