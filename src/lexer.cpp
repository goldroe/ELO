#include "lexer.h"

#include <stdlib.h>

void Lexer::advance() {
    if (*stream == '\n') {
        line_number++;
        column_number = 1;
    } else {
        column_number++;
    }

    stream_pos++;
    stream++;
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

void Lexer::scan() {
    Token tok;
begin:
    char *start = stream;
    int start_pos = stream_pos;

    switch (*stream) {
    case '#':
        advance();
        tok.type = TOKEN_HASH;
        break;
    case ';':
        advance();
        tok.type = TOKEN_SEMICOLON;
        break;
    case ':':
        advance();
        tok.type = TOKEN_COLON;
        if (*stream == ':') {
            advance();
            tok.type = TOKEN_COLON2;
        }
        break;
    case ',':
        advance();
        tok.type = TOKEN_COMMA;
        break;
    case '.':
        advance();
        tok.type = TOKEN_DOT;
        if (*stream == '.') {
            advance();
            tok.type = TOKEN_ELLIPSIS;
        }
        break;

    case '+':
        advance();
        tok.type = TOKEN_PLUS;
        if (*stream == '=') {
            advance();
            tok.type = TOKEN_ADD_ASSIGN;
        }
        break;
    case '-':
        advance();
        tok.type = TOKEN_MINUS;
        if (*stream == '=') {
            advance();
            tok.type = TOKEN_SUB_ASSIGN;
        }
        break;
    case '*':
        advance();
        tok.type = TOKEN_STAR;
        if (*stream == '=') {
            advance();
            tok.type = TOKEN_MUL_ASSIGN;
        }
        break;
    case '/':
        advance();
        tok.type = TOKEN_SLASH;
        if (*stream == '=') {
            advance();
            tok.type = TOKEN_DIV_ASSIGN;
        }
        break;
    case '%':
        advance();
        tok.type = TOKEN_PERCENT;
        if (*stream == '=') {
            advance();
            tok.type = TOKEN_MOD_ASSIGN;
        }
        break;
    case '&':
        break;
    case '|':
        break;
    case '^':
        break;
    case '~':
        break;
    case '(':
        advance();
        tok.type = TOKEN_LPAREN;
        break;
    case ')':
        advance();
        tok.type = TOKEN_RPAREN;
        break;
    case '[':
        advance();
        tok.type = TOKEN_LBRACKET;
        break;
    case ']':
        advance();
        tok.type = TOKEN_RBRACKET;
        break;
    case '{':
        advance();
        tok.type = TOKEN_LBRACE;
        break;
    case '}':
        advance();
        tok.type = TOKEN_RBRACE;
        break;
        
    case ' ': case '\n': case '\r': case '\f': case '\t':
        while (isspace(*stream)) {
            advance();
        }
        goto begin;
        break;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': case '_':
    {
        while (isalnum(*stream) || *stream == '_') {
            advance();
        }
        int len = stream_pos - start_pos;
        char *ident = (char *)malloc(len + 1);
        strncpy(ident, start, len);
        ident[len] = 0;

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
    case 0:
        tok.type = TOKEN_EOF;
        break;
    }

    token = tok;
}
