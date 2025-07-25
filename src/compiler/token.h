#if !defined(TOKEN_H)
#define TOKEN_H

#include "base/base_strings.h"

#include "constant_value.h"
#include "source_file.h"

struct Atom;

#define TOKEN_KINDS \
    TOKEN_KIND(TOKEN_UNKNOWN, "Unknown"), \
    TOKEN_KIND(TOKEN_EOF, "EOF"), \
    TOKEN_KIND(TOKEN_IDENT, "Ident"), \
    TOKEN_KIND(TOKEN_INTEGER, "Integer"), \
    TOKEN_KIND(TOKEN_FLOAT, "Float"), \
    TOKEN_KIND(TOKEN_STRING, "String"), \
    TOKEN_KIND(TOKEN_SEMI, ";"), \
    TOKEN_KIND(TOKEN_COLON, ":"), \
    TOKEN_KIND(TOKEN_COMMA, ","), \
    TOKEN_KIND(TOKEN_ARROW, "->"), \
    TOKEN_KIND(TOKEN_LPAREN, "("), \
    TOKEN_KIND(TOKEN_RPAREN, ")"), \
    TOKEN_KIND(TOKEN_LBRACE, "{"), \
    TOKEN_KIND(TOKEN_RBRACE, "}"), \
    TOKEN_KIND(TOKEN_UNINIT, "---"), \
                                                            \
    TOKEN_KIND(TOKEN_OPERATOR_BEGIN, "Operator__Begin"), \
    TOKEN_KIND(TOKEN_DOT, "."), \
    TOKEN_KIND(TOKEN_ELLIPSIS, ".."), \
    TOKEN_KIND(TOKEN_HASH, "#"), \
    TOKEN_KIND(TOKEN_PLUS, "+"), \
    TOKEN_KIND(TOKEN_MINUS, "-"), \
    TOKEN_KIND(TOKEN_STAR, "*"), \
    TOKEN_KIND(TOKEN_SLASH, "/"), \
    TOKEN_KIND(TOKEN_MOD, "%"), \
    TOKEN_KIND(TOKEN_INCREMENT, "++"), \
    TOKEN_KIND(TOKEN_DECREMENT, "--"), \
                                                        \
    TOKEN_KIND(TOKEN_ASSIGN_BEGIN, "Assign__Begin"), \
    TOKEN_KIND(TOKEN_EQ, "="), \
    TOKEN_KIND(TOKEN_PLUS_EQ, "+="), \
    TOKEN_KIND(TOKEN_MINUS_EQ, "-="), \
    TOKEN_KIND(TOKEN_STAR_EQ, "*="), \
    TOKEN_KIND(TOKEN_SLASH_EQ, "/="), \
    TOKEN_KIND(TOKEN_MOD_EQ, "%="), \
    TOKEN_KIND(TOKEN_XOR_EQ, "^="), \
    TOKEN_KIND(TOKEN_BAR_EQ, "|="), \
    TOKEN_KIND(TOKEN_AMPER_EQ, "&="), \
    TOKEN_KIND(TOKEN_LSHIFT_EQ, "<<="), \
    TOKEN_KIND(TOKEN_RSHIFT_EQ, ">>="), \
    TOKEN_KIND(TOKEN_ASSIGN_END, "Assign__End"), \
                                                 \
    TOKEN_KIND(TOKEN_EQ2, "=="), \
    TOKEN_KIND(TOKEN_NEQ, "!="), \
    TOKEN_KIND(TOKEN_LT, "<"), \
    TOKEN_KIND(TOKEN_LTEQ, "<="), \
    TOKEN_KIND(TOKEN_GT, ">"), \
    TOKEN_KIND(TOKEN_GTEQ, ">="), \
    TOKEN_KIND(TOKEN_BANG, "!"), \
    TOKEN_KIND(TOKEN_AMPER, "&"), \
    TOKEN_KIND(TOKEN_BAR, "|"), \
    TOKEN_KIND(TOKEN_AND, "&&"), \
    TOKEN_KIND(TOKEN_OR, "||"), \
    TOKEN_KIND(TOKEN_XOR, "^"), \
    TOKEN_KIND(TOKEN_SQUIGGLE, "~"), \
    TOKEN_KIND(TOKEN_LSHIFT, "<<"), \
    TOKEN_KIND(TOKEN_RSHIFT, ">>"), \
    TOKEN_KIND(TOKEN_LBRACKET, "["), \
    TOKEN_KIND(TOKEN_RBRACKET, "]"), \
    TOKEN_KIND(TOKEN_DOT_STAR, ".*"), \
    TOKEN_KIND(TOKEN_OPERATOR_END, "Operator__End"), \
                                                        \
    TOKEN_KIND(TOKEN_KEYWORD_BEGIN, "Keyword__Begin"), \
    TOKEN_KIND(TOKEN_ENUM, "enum"), \
    TOKEN_KIND(TOKEN_STRUCT, "struct"), \
    TOKEN_KIND(TOKEN_UNION, "union"), \
    TOKEN_KIND(TOKEN_IF, "if"), \
    TOKEN_KIND(TOKEN_ELSE, "else"), \
    TOKEN_KIND(TOKEN_IFCASE, "ifcase"), \
    TOKEN_KIND(TOKEN_CASE, "case"), \
    TOKEN_KIND(TOKEN_DO, "do"), \
    TOKEN_KIND(TOKEN_WHILE, "while"), \
    TOKEN_KIND(TOKEN_FOR, "for"), \
    TOKEN_KIND(TOKEN_BREAK, "break"), \
    TOKEN_KIND(TOKEN_CONTINUE, "continue"), \
    TOKEN_KIND(TOKEN_FALLTHROUGH, "fallthrough"), \
    TOKEN_KIND(TOKEN_RETURN, "return"), \
    TOKEN_KIND(TOKEN_DEFER, "defer"), \
    TOKEN_KIND(TOKEN_CAST, "cast"), \
    TOKEN_KIND(TOKEN_OPERATOR, "operator"), \
    TOKEN_KIND(TOKEN_IN, "in"), \
    TOKEN_KIND(TOKEN_SIZEOF, "sizeof"), \
    TOKEN_KIND(TOKEN_TYPEOF, "typeof"), \
    TOKEN_KIND(TOKEN_KEYWORD_END, "Keyword__End"), \

enum Token_Kind {
#define TOKEN_KIND(K,S) K
    TOKEN_KINDS
#undef TOKEN_KIND
    TOKEN_COUNT
};

extern const char *token_strings[TOKEN_COUNT];

#define LITERAL_KINDS \
    LITERAL_KIND(LITERAL_U8,  "u8"),   \
    LITERAL_KIND(LITERAL_U16, "u16"), \
    LITERAL_KIND(LITERAL_U32, "u32"), \
    LITERAL_KIND(LITERAL_U64, "u64"), \
    LITERAL_KIND(LITERAL_I8,  "i8"),   \
    LITERAL_KIND(LITERAL_I16, "i16"), \
    LITERAL_KIND(LITERAL_I32, "i32"), \
    LITERAL_KIND(LITERAL_I64, "i64"), \
    LITERAL_KIND(LITERAL_F32, "f32"), \
    LITERAL_KIND(LITERAL_F64, "f64"), \

enum Literal_Kind {
#define LITERAL_KIND(K,S) K
    LITERAL_KINDS
#undef LITERAL_KIND
    LITERAL_DEFAULT,
};

struct Suffix_Literal {
    String string;
    Literal_Kind literal;
};

extern Suffix_Literal suffix_literals[];

struct Source_Pos {
    Source_File *file;
    u64 line;
    u64 col;
    u64 index;
};

struct Token {
    Token_Kind kind = TOKEN_UNKNOWN;
    Source_Pos start = {};
    Source_Pos end = {};
    String string;

    union {
        Atom *name;
        struct {
            Constant_Value value;
            Literal_Kind literal_kind;
        };
    };
};

const char *string_from_token(Token_Kind token);


#endif //TOKEN_H
