#ifndef LEXER_H
#define LEXER_H

#include <ctype.h>
#include <string.h>

#include "types.h"
#include "path.h"

struct Atom {
    int64 count;
    Atom *next;
    char name[];
};

Atom *make_atom(const char *str, int len);
Atom *make_atom(const char *str);
bool atoms_match(Atom *first, Atom *last);

#define TOKENS() \
    TOK(Token_EOF, "EOF"), \
    TOK(Token_None, ""), \
    \
    TOK(Token_BuiltinFirst, "beginning of builtin types"), \
    TOK(Token_Bool, "bool"), \
    TOK(Token_Int, "int"), \
    TOK(Token_Int8, "int8"), \
    TOK(Token_Int16, "int16"), \
    TOK(Token_Int32, "int32"), \
    TOK(Token_Int64, "int64"), \
    TOK(Token_Uint, "uint"), \
    TOK(Token_Uint8, "uint8"), \
    TOK(Token_Uint16, "uint16"), \
    TOK(Token_Uint32, "uint32"), \
    TOK(Token_Uint64, "uint64"), \
    TOK(Token_Float, "float"), \
    TOK(Token_Float32, "float32"), \
    TOK(Token_Float64, "float64"), \
    TOK(Token_String, "string"), \
    TOK(Token_BuiltinLast, "end of builtin types"), \
    \
    TOK(Token_KeywordFirst, "beginning of keywords"), \
    TOK(Token_Struct, "struct"), \
    TOK(Token_Enum, "enum"), \
    TOK(Token_If, "if"), \
    TOK(Token_Else, "else"), \
    TOK(Token_While, "while"), \
    TOK(Token_Do, "do"), \
    TOK(Token_For, "for"), \
    TOK(Token_Case, "case"), \
    TOK(Token_Continue, "continue"), \
    TOK(Token_Break, "break"), \
    TOK(Token_Return, "return"), \
    TOK(Token_Const, "const"), \
    TOK(Token_True, "true"), \
    TOK(Token_False, "false"), \
    TOK(Token_KeywordLast, "end of keywords"), \
    \
    TOK(Token_Hash, "#"), \
    TOK(Token_Colon, ":"), \
    TOK(Token_Colon2, "::"), \
    TOK(Token_Semicolon, ";"), \
    TOK(Token_Comma, ","), \
    TOK(Token_Dot, "."), \
    TOK(Token_Ellipsis, ".."), \
    TOK(Token_Arrow, "->"), \
    \
    TOK(Token_Plus, "+"), \
    TOK(Token_Minus, "-"), \
    TOK(Token_Star, "*"), \
    TOK(Token_Slash, "/"), \
    TOK(Token_Percent, "%"), \
    TOK(Token_Amper, "&"), \
    TOK(Token_Bar, "|"), \
    TOK(Token_Xor, "^"), \
    TOK(Token_Tilde, "~"), \
    \
    TOK(Token_Bang, "!"), \
    TOK(Token_Equal, "=="), \
    TOK(Token_Neq, "!="), \
    TOK(Token_And, "&&"), \
    TOK(Token_Or, "||"), \
    TOK(Token_Lt, "<"), \
    TOK(Token_Lteq, "<="), \
    TOK(Token_Gt, ">"), \
    TOK(Token_Gteq, ">="), \
    TOK(Token_Lshift, "<<"), \
    TOK(Token_Rshift, ">>"), \
    \
    TOK(Token_Question, "?"), \
    \
    TOK(Token_AssignFirst, "beginning of assign operators"), \
    TOK(Token_Assign, "="), \
    TOK(Token_ColonAssign, ":="), \
    TOK(Token_AddAssign, "+="), \
    TOK(Token_SubAssign, "-="), \
    TOK(Token_MulAssign, "*="), \
    TOK(Token_DivAssign, "/="), \
    TOK(Token_ModAssign, "%="), \
    TOK(Token_AndAssign, "&="), \
    TOK(Token_OrAssign, "|="), \
    TOK(Token_NotAssign, "~="), \
    TOK(Token_XorAssign, "^="), \
    TOK(Token_AssignLast, "end of assign operators"), \
    \
    TOK(Token_OpenParen, "("), \
    TOK(Token_CloseParen, ")"), \
    TOK(Token_OpenBracket, "["), \
    TOK(Token_CloseBracket, "]"), \
    TOK(Token_OpenBrace, "{"), \
    TOK(Token_CloseBrace, "}"), \
    \
    TOK(Token_Ident, "identifier"), \
    TOK(Token_Strlit, "string"), \
    TOK(Token_Intlit, "integer"), \
    TOK(Token_Floatlit, "float"), \
    TOK(Token_Terminator, "")

enum Token_Type {
#define TOK(Tok, Str) Tok
    TOKENS()
#undef TOK
};
inline Token_Type& operator ++(Token_Type &a, int) { a = (Token_Type)(((int)a) + 1); return a; }

extern const char *_token_type_strings[Token_Terminator + 1];
inline const char *token_type_to_string(Token_Type type) {
    return _token_type_strings[type];
}

void init_keywords();
void init_atom_map();


inline bool is_assign_operator(Token_Type op) {
    return (op > Token_AssignFirst && op < Token_AssignLast);
}

inline bool is_unary_operator(Token_Type op) {
    switch (op) {
    default:
        return false;
    case Token_Minus:
    case Token_Plus:
    case Token_Star:
    case Token_Amper:
    case Token_Bang:
    case Token_Tilde:
    case Token_Xor:
        return true;
    }
}

struct Token {
    Token_Type type = Token_None;

    union {
        Atom *name;
        uint64 intlit = 0;
        float64 floatlit;
        char *strlit;
    };
};

struct Lexer {
    char *source_name = nullptr;
    char *source_text = nullptr;

    char *stream = nullptr;

    int l0 = 1;
    int c0 = 0;
    int l1 = 1;
    int c1 = 0;

    Token token;

    Lexer(char *source_file_name) {
        source_name = source_file_name;
        source_text = read_entire_file(source_file_name);
        stream = source_text;
        next_token();
    }

    void error(const char *fmt, ...);


    inline bool is_token(Token_Type type) {
        return token.type == type; 
    }

    Token peek_token() {
        char *_stream = stream;
        int _l0 = l0;
        int _l1 = l1;
        int _c0 = c0;
        int _c1 = c1;
        Token tok = scan();
        // rewind
        stream = _stream;
        l0 = _l0;
        l1 = _l1;
        c0 = _c0;
        c1 = _c1;
        return tok;
    }

    inline bool match_token(Token_Type type) {
        if (is_token(type)) {
            next_token();
            return true;
        }
        return false;
    }

    inline void next_token() {
        Token tok = scan();
        token = tok;
    }

    void advance() {
        if (*stream == '\r') {
            stream++;
            if (*stream == '\n') stream++;
            l1++;
            c1 = 0;
        } else if (*stream == '\n') {
            stream++;
            l1++;
            c1 = 0;
        } else {
            stream++;
            c1++;
        }
    }

    void eat_line() {
        while (*stream != '\n') {
            advance();
        }
    }

    int64 scan_integer();
    float64 scan_float();
    Token scan();
};

#endif // LEXER_H
