#ifndef LEXER_H
#define LEXER_H

#include <ctype.h>
#include <string.h>

#include "types.h"
#include "path.h"


#define TOKENS() \
    TOK(TOKEN_EOF, ""), \
    TOK(TOKEN_NONE, ""), \
    \
    TOK(TOKEN_BUILTIN_BEGIN, "beginning of builtin types"), \
    TOK(TOKEN_BOOL, "bool"), \
    TOK(TOKEN_INT, "int"), \
    TOK(TOKEN_INT8, "int8"), \
    TOK(TOKEN_INT16, "int16"), \
    TOK(TOKEN_INT32, "int32"), \
    TOK(TOKEN_INT64, "int64"), \
    TOK(TOKEN_UINT, "uint"), \
    TOK(TOKEN_UINT8, "uint8"), \
    TOK(TOKEN_UINT16, "uint16"), \
    TOK(TOKEN_UINT32, "uint32"), \
    TOK(TOKEN_UINT64, "uint64"), \
    TOK(TOKEN_FLOAT, "float"), \
    TOK(TOKEN_FLOAT32, "float32"), \
    TOK(TOKEN_FLOAT64, "float64"), \
    TOK(TOKEN_STRING, "string"), \
    TOK(TOKEN_BUILTIN_END, "end of builtin types"), \
    \
    TOK(TOKEN_KEYWORD_BEGIN, "beginning of keywords"), \
    TOK(TOKEN_STRUCT, "struct"), \
    TOK(TOKEN_ENUM, "enum"), \
    TOK(TOKEN_IF, "if"), \
    TOK(TOKEN_ELSE, "else"), \
    TOK(TOKEN_WHILE, "while"), \
    TOK(TOKEN_DO, "do"), \
    TOK(TOKEN_FOR, "for"), \
    TOK(TOKEN_CASE, "case"), \
    TOK(TOKEN_CONTINUE, "continue"), \
    TOK(TOKEN_BREAK, "break"), \
    TOK(TOKEN_RETURN, "return"), \
    TOK(TOKEN_CONST, "const"), \
    TOK(TOKEN_TRUE, "true"), \
    TOK(TOKEN_FALSE, "false"), \
    TOK(TOKEN_KEYWORD_END, "end of keywords"), \
    \
    TOK(TOKEN_HASH, "#"), \
    TOK(TOKEN_COLON, ":"), \
    TOK(TOKEN_COLON2, "::"), \
    TOK(TOKEN_SEMICOLON, ";"), \
    TOK(TOKEN_COMMA, ","), \
    TOK(TOKEN_DOT, "."), \
    TOK(TOKEN_ELLIPSIS, ".."), \
    \
    TOK(TOKEN_PLUS, "+"), \
    TOK(TOKEN_MINUS, "-"), \
    TOK(TOKEN_STAR, "*"), \
    TOK(TOKEN_SLASH, "/"), \
    TOK(TOKEN_PERCENT, "%"), \
    TOK(TOKEN_AMPER, "&"), \
    TOK(TOKEN_BAR, "|"), \
    TOK(TOKEN_XOR, "^"), \
    TOK(TOKEN_TILDE, "~"), \
    \
    TOK(TOKEN_BANG, "!"), \
    TOK(TOKEN_EQUAL, "=="), \
    TOK(TOKEN_NEQ, "!="), \
    TOK(TOKEN_AND, "&&"), \
    TOK(TOKEN_OR, "||"), \
    TOK(TOKEN_LT, "<"), \
    TOK(TOKEN_LTEQ, "<="), \
    TOK(TOKEN_GT, ">"), \
    TOK(TOKEN_GTEQ, ">="), \
    TOK(TOKEN_LSHIFT, "<<"), \
    TOK(TOKEN_RSHIFT, ">>"), \
    \
    TOK(TOKEN_QUESTION, "?"), \
    \
    TOK(TOKEN_ASSIGN, "="), \
    TOK(TOKEN_COLON_ASSIGN, ":="), \
    TOK(TOKEN_ADD_ASSIGN, "+="), \
    TOK(TOKEN_SUB_ASSIGN, "-="), \
    TOK(TOKEN_MUL_ASSIGN, "*="), \
    TOK(TOKEN_DIV_ASSIGN, "/="), \
    TOK(TOKEN_MOD_ASSIGN, "%="), \
    TOK(TOKEN_AND_ASSIGN, "&="), \
    TOK(TOKEN_OR_ASSIGN, "|="), \
    TOK(TOKEN_NOT_ASSIGN, "~="), \
    TOK(TOKEN_XOR_ASSIGN, "^="), \
    \
    TOK(TOKEN_LPAREN, "("), \
    TOK(TOKEN_RPAREN, ")"), \
    TOK(TOKEN_LBRACKET, "["), \
    TOK(TOKEN_RBRACKET, "]"), \
    TOK(TOKEN_LBRACE, "{"), \
    TOK(TOKEN_RBRACE, "}"), \
    \
    TOK(TOKEN_IDENT, "identifier"), \
    TOK(TOKEN_STRLIT, "string"), \
    TOK(TOKEN_INTLIT, "integer"), \
    TOK(TOKEN_FLOATLIT, "float"), \
    TOK(TOKEN_TERMINATOR, "")

enum Token_Type {
#define TOK(Tok, Str) Tok
    TOKENS()
#undef TOK
};

struct Token {
    Token_Type type = TOKEN_NONE;
    Source_Range source_range = {};

    union {
        uint64 intlit = 0;
        float64 floatlit;
        char *strlit;
    };
};

struct Lexer {
    char *source_name = nullptr;
    char *file_name = nullptr;
    char *source_text = nullptr;

    char *stream = nullptr;
    int stream_pos = 0;

    int line_number = 0;
    int column_number = 1;

    Token token;

    Lexer(char *source_file_name) {
        source_name = source_file_name;
        file_name = source_file_name;
        source_text = read_entire_file(source_file_name);
        stream = source_text;

        scan();
    }

    void error(const char *fmt, ...);
    void advance();
    int64 scan_integer();
    float64 scan_float();
    void scan();
};

#endif // LEXER_H
