#ifndef PARSER_H
#define PARSER_H

#include "core.h"
#include "lexer.h"
#include "array.h"

enum Ast_Kind {
    AstKind_Nil,

    AstKind_Root,
    AstKind_TypeDefinition,
    AstKind_TypeInfo,
    AstKind_Block,

    AstKind_Declaration,
    AstKind_Procedure,
    AstKind_Variable,
    AstKind_Struct,
    AstKind_StructField,
    AstKind_Union,
    AstKind_Enum,
    AstKind_EnumField,

    AstKind_Statement,
    AstKind_If,
    AstKind_While,
    AstKind_For,
    AstKind_Return,
    AstKind_Break,
    AstKind_Continue,
    AstKind_DeclarationStatement,
    AstKind_ExpressionStatement,
    AstKind_BlockStatement,

    AstKind_Expression,
    AstKind_BinaryExpression,
    AstKind_UnaryExpression,
    AstKind_CastExpression,
    AstKind_IndexExpression,
    AstKind_FieldExpression,
    AstKind_CallExpression,
    AstKind_RangeExpression,
    AstKind_Literal,
    AstKind_Ident,
};

struct Ast_Statement;
struct Ast_Expression;
struct Ast_Declaration;
struct Ast_Ident;
struct Ast_Block;
struct Ast_Type_Info;
struct Ast_Enum_Field;

struct Ast {
    Ast_Kind kind = AstKind_Nil;
    Source_Loc start = {};
    Source_Loc end = {};
    bool32 poisoned = false;
};

enum Type_Kind {
    TypeKind_Nil,
    TypeKind_Pointer,
    TypeKind_Array,
    TypeKind_Struct,
    TypeKind_Enum,
    TypeKind_Procedure,
    TypeKind_String,

    TypeKind_Bool,
    TypeKind_Int,
    TypeKind_Int8,
    TypeKind_Int16,
    TypeKind_Int32,
    TypeKind_Int64,
    TypeKind_Uint,
    TypeKind_Uint8,
    TypeKind_Uint16,
    TypeKind_Uint32,
    TypeKind_Uint64,
    TypeKind_Float32,
    TypeKind_Float64,
};

enum Literal_Flags {
    LiteralFlag_Number = (1 << 0),
    LiteralFlag_String = (1 << 1),
    LiteralFlag_Float  = (1 << 2),
};
DEFINE_ENUM_FLAG_OPERATORS(Literal_Flags);

enum Type_Info_Flags {
    TypeInfoFlag_Nil     = 0,
    TypeInfoFlag_Basic   = (1<<0),
    TypeInfoFlag_Integer = (1<<1),
    TypeInfoFlag_Float   = (1<<2),
    TypeInfoFlag_Signed  = (1<<3),
};
DEFINE_ENUM_FLAG_OPERATORS(Type_Info_Flags);

enum Expr_Flags {
    ExprFlag_Lvalue = (1<<0),
};
DEFINE_ENUM_FLAG_OPERATORS(Expr_Flags);

enum Decl_Flags {
    DeclFlag_Global = (1 << 0),
};
DEFINE_ENUM_FLAG_OPERATORS(Decl_Flags);

enum Type_Defn_Flags {
    TypeDefnFlag_Ident        = (1<<0),
    TypeDefnFlag_Pointer      = (1<<1),
    TypeDefnFlag_Array        = (1<<2),
    TypeDefnFlag_DynamicArray = (1<<3),
};
DEFINE_ENUM_FLAG_OPERATORS(Type_Defn_Flags);

struct Field_Type {
    Atom *name;
    Ast_Type_Info *type;
    uint32 offset;
};

struct Param_Type {
    Atom *name;
    Ast_Type_Info *type;
};

struct Ast_Type_Info : Ast {
    Ast_Type_Info() { kind = AstKind_TypeInfo; }
    Type_Kind type_kind;
    Type_Info_Flags type_flags;
    uint32 bits;
    uint32 align;
    uint32 padding;
    Ast_Type_Info *base;
    union {
        struct {
            Atom *name;
            Array<Field_Type> fields;
        } aggregate;
        struct {
            Array<Param_Type> params;
            Ast_Type_Info *return_type;
        } procedure;
        struct {
            Atom *name;
            Array<Ast_Enum_Field*> fields;
        } enumerated;
    };
};

struct Ast_Expression : Ast {
    Ast_Expression() { kind = AstKind_Expression; }
    Expr_Flags expr_flags;
    Ast_Type_Info *inferred_type;
};

struct Ast_Declaration : Ast {
    Ast_Declaration() { kind = AstKind_Declaration; }
    Decl_Flags declaration_flags;
    Ast_Ident *ident;
    Ast_Type_Info *inferred_type;

    bool resolved = false;
    bool resolving = false;
};

struct Ast_Statement : Ast {
    Ast_Statement() { kind = AstKind_Statement; }
};

struct Ast_Root : Ast {
    Ast_Root() { kind = AstKind_Root; }
    Array<Ast_Declaration *> declarations;
};

struct Ast_Type_Definition : Ast {
    Ast_Type_Definition() {kind = AstKind_TypeDefinition; }
    Ast_Type_Definition *base;
    Type_Defn_Flags defn_flags;
    Ast_Ident *ident;
    Ast_Expression *array_size;
};

struct Ast_Block : Ast {
    Ast_Block() { kind = AstKind_Block; }
    Array<Ast_Statement *> statements;
};

struct Ast_Variable : Ast_Declaration {
    Ast_Variable() { kind = AstKind_Variable; }
    Ast_Type_Definition *type_definition;
    Ast_Expression *initializer;
};

struct Ast_Procedure_Declaration : Ast_Declaration {
    Ast_Procedure_Declaration() { kind = AstKind_Procedure; }
    Array<Ast_Variable*> parameters;
    Ast_Type_Definition *return_type;
    Ast_Block *body;
};

struct Ast_Struct_Field : Ast {
    Ast_Struct_Field() { kind = AstKind_StructField; }
    Atom *name;
    Ast_Type_Definition *type_definition;
    Ast_Type_Info *inferred_type;
};

struct Ast_Struct_Declaration : Ast_Declaration {
    Ast_Struct_Declaration() { kind = AstKind_Struct; }
    Array<Ast_Struct_Field*> fields;
};

struct Ast_Enum_Field : Ast {
    Ast_Enum_Field() { kind = AstKind_EnumField; }
    Atom *name;
    int64 value;
};

struct Ast_Enum_Declaration : Ast_Declaration {
    Ast_Enum_Declaration() { kind = AstKind_Enum; }
    Array<Ast_Enum_Field*> fields;
};

struct Ast_Declaration_Statement : Ast_Statement {
    Ast_Declaration_Statement() { kind = AstKind_DeclarationStatement; }
    Ast_Declaration *declaration;
};

struct Ast_Expression_Statement : Ast_Statement {
    Ast_Expression_Statement() { kind = AstKind_ExpressionStatement; }
    Ast_Expression *expression;
};

struct Ast_Block_Statement : Ast_Statement {
    Ast_Block_Statement() { kind = AstKind_BlockStatement; }
    Ast_Block *block;
};

struct Ast_If : Ast_Statement {
    Ast_If() { kind = AstKind_If; }
    Ast_Expression *condition;
    Ast_Block *block;
    Ast_If *next;
};

struct Ast_While : Ast_Statement {
    Ast_While() { kind = AstKind_While; }
    Ast_Expression *condition;
    Ast_Block *block;
};

struct Ast_Cast_Expression : Ast_Expression {
    Ast_Cast_Expression() { kind = AstKind_CastExpression; }
    Ast_Type_Definition *type_cast;
};

struct Ast_Range_Expression : Ast_Expression {
    Ast_Range_Expression() { kind = AstKind_RangeExpression; }
    Ast_Expression *first;
    Ast_Expression *last;
};

struct Ast_For : Ast_Statement {
    Ast_For() { kind = AstKind_For; }
    Ast_Ident *iterator;
    Ast_Range_Expression *range;
    Ast_Block *block;
};

struct Ast_Break : Ast_Statement {
    Ast_Break() { kind = AstKind_Break; }
};

struct Ast_Return : Ast_Statement {
    Ast_Return() { kind = AstKind_Return; }
    Ast_Expression *expression;
};

struct Ast_Binary_Expression : Ast_Expression {
    Ast_Binary_Expression() { kind = AstKind_BinaryExpression; }
    Token_Type op;
    Ast_Expression *lhs;
    Ast_Expression *rhs;
};

struct Ast_Unary_Expression : Ast_Expression {
    Ast_Unary_Expression() { kind = AstKind_UnaryExpression; }
    Token_Type op;
    Ast_Expression *expression;
};

struct Ast_Index_Expression : Ast_Expression {
    Ast_Index_Expression() { kind = AstKind_IndexExpression; }
    Ast_Expression *array;
    Ast_Expression *index;
};

struct Ast_Call_Expression : Ast_Expression {
    Ast_Call_Expression() { kind = AstKind_CallExpression; }
    Ast_Expression *operand;
    Array<Ast_Expression *> arguments;
};

struct Ast_Field_Expression : Ast_Expression {
    Ast_Field_Expression() { kind = AstKind_FieldExpression; }
    Ast_Expression *operand;
    Ast_Expression *field;
};

struct Ast_Literal : Ast_Expression {
    Ast_Literal() { kind = AstKind_Literal; }
    Literal_Flags literal_flags;
    union {
        uint64 int_value;
        float64 float_value;
        char *string_value;
    };
};

struct Ast_Ident : Ast_Expression {
    Ast_Ident() { kind = AstKind_Ident; }
    Atom *name;
};

struct Parser {
    Ast_Root *root = nullptr;
    Lexer *lexer = nullptr;

    int error_count = 0;

    Parser(char *source_name) {
        lexer = new Lexer(source_name);

        root = parse_root();
    }

    Source_Loc get_start_loc() {
        Source_Loc result = {lexer->l0, lexer->c0};
        return result;
    }

    Source_Loc get_end_loc() {
        Source_Loc result = {lexer->l1, lexer->c1};
        return result;
    }

    void start_loc(Ast *ast) {
        Source_Loc loc = get_start_loc();
        ast->start = loc;
    }

    void end_loc(Ast *ast) {
        Source_Loc loc = get_end_loc();
        ast->end = loc;
    }

    void error(const char *fmt, ...);
    bool expect(Token_Type type);

    Ast_Ident *parse_ident();
    Ast_Expression *parse_expression();
    Ast_Cast_Expression *parse_cast_expression();
    Ast_Expression *parse_primary_expression();
    Ast_Expression *parse_unary_expression();
    Ast_Expression *parse_operand();
    Ast_Expression *parse_binary_expression(Ast_Expression *lhs, int precedence);
    Ast_Range_Expression *parse_range_expression();

    Ast_Statement *parse_statement();
    Ast_Statement *parse_init_statement(Ast_Expression *lhs);
    Ast_Statement *parse_simple_statement();
    Ast_If *parse_if_statement();
    Ast_While *parse_while_statement();
    Ast_For *parse_for_statement();
    Ast_Break *parse_break_statement();
    Ast_Return *parse_return_statement();

    Ast_Declaration *parse_declaration();
    Ast_Variable *parse_variable_declaration(Ast_Ident *identfier);
    Ast_Procedure_Declaration *parse_procedure_declaration(Ast_Ident *ident);
    Ast_Struct_Declaration *parse_struct_declaration(Ast_Ident *ident);
    Ast_Enum_Declaration *parse_enum_declaration(Ast_Ident *ident);

    Ast_Block *parse_block();
    Ast_Type_Definition *parse_type_definition();
    Ast_Root *parse_root();
};

Ast_Ident *make_ident(Atom *name);
Ast_Declaration *make_declaration(Ast_Ident *ident);

char *type_definition_to_string(Ast_Type_Definition *type_definition);
char *type_to_string(Ast_Type_Info *type);

Ast_Type_Info *make_type_info(Type_Kind kind, Type_Info_Flags flags, int bits, Ast_Type_Info *base = nullptr);


#endif // PARSER_H
