#include "lexer.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

const char *_token_type_strings[TOKEN_TERMINATOR + 1] = {
#define TOK(Tok, Str) Str
    TOKENS()
#undef TOK
};

void Lexer::error(const char *fmt, ...) {
    printf("error: ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

int64 Lexer::scan_integer() {
    int64 result = 0;
    int base = 10;

    while (isdigit(*stream)) {
        int digit = *stream - '0';
        advance();
        result = result * base + digit;
    }
    return result;
}

float64 Lexer::scan_float() {
    const char *start = stream;
    while (isdigit(*stream)) {
        advance();
    }
    if (*stream == '.') {
        advance();
    } else {
        // @todo Invalid character in float
    }

    while (isdigit(*stream)) {
        advance();
    }
    if (*stream == 'f') {
        advance();
    }

    double result = strtod(start, NULL);
    return result;
}

Token Lexer::scan() {
    Token tok;
begin:
    char *start = stream;
    int start_pos = stream_pos;

#define CASE1(C0, T0) \
    case C0: \
        advance(); \
        tok.type = T0; \
        break; \

#define CASE2(C0, T0, C1, T1) \
    case C0: \
        advance(); \
        tok.type = T0; \
        if (*stream == C1) { \
            advance(); \
            tok.type = T1; \
        } \
        break; \

#define CASE3(C0, T0, C1, T1, C2, T2) \
    case C0: \
        advance(); \
        tok.type = T0; \
        if (*stream == C1) { \
            advance(); \
            tok.type = T1; \
        } else if (*stream == C2) { \
            advance(); \
            tok.type = T2; \
        } \
        break; \
    
    switch (*stream) {
        CASE1(';', TOKEN_SEMICOLON);
        CASE1('#', TOKEN_HASH);
        CASE1(',', TOKEN_COMMA);
        CASE2('.', TOKEN_DOT, '.', TOKEN_ELLIPSIS);

        CASE1('(', TOKEN_LPAREN);
        CASE1(')', TOKEN_RPAREN);
        CASE1('[', TOKEN_LBRACKET);
        CASE1(']', TOKEN_RBRACKET);
        CASE1('{', TOKEN_LBRACE);
        CASE1('}', TOKEN_RBRACE);

        CASE1('?', TOKEN_QUESTION);

        CASE2('+', TOKEN_PLUS, '=', TOKEN_ADD_ASSIGN);
        CASE3('-', TOKEN_MINUS, '=', TOKEN_SUB_ASSIGN, '>', TOKEN_ARROW);
        CASE2('*', TOKEN_STAR, '=', TOKEN_MUL_ASSIGN);
        CASE2('/', TOKEN_SLASH, '=', TOKEN_DIV_ASSIGN);
        CASE2('%', TOKEN_PERCENT, '=', TOKEN_MOD_ASSIGN);
        CASE2('^', TOKEN_XOR, '=', TOKEN_XOR_ASSIGN);
        CASE2('~', TOKEN_TILDE, '=', TOKEN_NOT_ASSIGN);

        CASE2('!', TOKEN_BANG, '=', TOKEN_NEQ);
        CASE2('=', TOKEN_ASSIGN, '=', TOKEN_EQUAL);

        CASE3(':', TOKEN_COLON, ':', TOKEN_COLON2, '=', TOKEN_COLON_ASSIGN);

    case '&':
        advance();
        tok.type = TOKEN_AMPER;
        if (*stream == '&') {
            advance();
            tok.type = TOKEN_AND;
        } else if (*stream == '=') {
            advance();
            tok.type = TOKEN_AND_ASSIGN;
        }
        break;
    case '|':
        advance();
        tok.type = TOKEN_BAR;
        if (*stream == '|') {
            advance();
            tok.type = TOKEN_OR;
        } else if (*stream == '=') {
            advance();
            tok.type = TOKEN_OR_ASSIGN;
        }
        break;
    case '<':
        advance();
        tok.type = TOKEN_LT;
        if (*stream == '=') {
            advance();
            tok.type = TOKEN_LTEQ;
        } else if (*stream == '<') {
            advance();
            tok.type = TOKEN_LSHIFT;
        }
        break;
    case '>':
        advance();
        tok.type = TOKEN_GT;
        if (*stream == '=') {
            advance();
            tok.type = TOKEN_GTEQ;
        } else if (*stream == '>') {
            advance();
            tok.type = TOKEN_RSHIFT;
        }
        break;

    case '"':
    {
        advance();
        for (;;) {
            switch (*stream) {
            case '"':
                advance();
                goto string_end;
            case '\n':
                error("newline in string literal");
                goto string_end;
            case 0:
                error("unexpected end of file in string literal");
                goto string_end;
            }
            advance();
        }
    string_end:

        int len = stream_pos - start_pos - 2;
        tok.type = TOKEN_STRLIT;
        tok.strlit = (char *)malloc(len + 1);
        strncpy(tok.strlit, start + 1, len);
        tok.strlit[len] = 0;
        break;
    }
        
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': case '_':
    {
        while (isalnum(*stream) || *stream == '_') {
            advance();
        }
        int len = stream_pos - start_pos;
        char *ident = (char *)malloc(len + 1);
        strncpy(ident, start, len);
        ident[len] = 0;

        // @todo implement string hashmap for string interning and separate keyword hashmap for scanning keywords
        tok.type = TOKEN_IDENT;
        tok.strlit = ident;
        break;
    }
    
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
    {
        while (isalnum(*stream)) {
            advance();
        }
        if (*stream == '.') {
            stream = start;
            stream_pos = start_pos;
            tok.type = TOKEN_FLOATLIT;
            tok.floatlit = scan_float();
        } else {
            stream = start;
            stream_pos = start_pos;
            tok.type = TOKEN_INTLIT;
            tok.intlit = scan_integer();
        }
        break;
    }

    case ' ': case '\n': case '\r': case '\f': case '\t':
        while (isspace(*stream)) {
            advance();
        }
        goto begin;
        break;

    case 0:
        tok.type = TOKEN_EOF;
        break;
    }
#undef CASE1
#undef CASE2

    return tok;
}
