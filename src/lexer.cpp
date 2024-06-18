#include "lexer.h"
#include "core.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

struct Atom_Map {
    Atom **atoms;
    int count;
};

const char *_token_type_strings[Token_Terminator + 1] = {
#define TOK(Tok, Str) Str
    TOKENS()
#undef TOK
};

Atom_Map *atom_map;
Arena atom_arena;

uint64 hash_djb2(unsigned const char *str, size_t len) {
    uint64_t hash = 5381;
    int c;
    for (size_t i = 0; i < len; i++) {
        c = str[i];
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

Atom_Map *make_atom_map() {
    Atom_Map *map = (Atom_Map *)malloc(sizeof(Atom_Map));
    map->count = 256;
    map->atoms = (Atom **)calloc(map->count, sizeof(Atom *));
    return map;
}

void init_atom_map() {
    atom_map = make_atom_map();
    atom_arena = make_arena();
}

Atom *make_atom(const char *str, int len) {
    uint64 hash = hash_djb2((unsigned const char *)str, len);
    uint64 index = hash % atom_map->count;

    Atom *first = atom_map->atoms[index];
    for (Atom *it = first; it; it = it->next) {
        if (it->count == len && strncmp(it->name, str, len) == 0) {
            // printf("FOUND ATOM: '%s' #%llu %p\n", it->name, hash, it);
            return it;
        }
    }

    Atom *atom = (Atom *)arena_alloc(&atom_arena, offsetof(Atom, name) + len + 1);
    strncpy(atom->name, str, len);
    atom->name[len] = 0;
    atom->count = len;
    atom->next = first;
    atom_map->atoms[index] = atom;
    // printf("NEW ATOM: '%s' #%llu %p\n", atom->name, hash, atom);
    return atom;
}

Atom *make_atom(const char *str) {
    return make_atom(str, (int)strlen(str));
}

bool atoms_match(Atom *first, Atom *last) {
    return first == last;
}

struct Keyword_Entry {
    const char *key;
    Token_Type token;
    Keyword_Entry *next;
};

struct Keyword_Table {
    Keyword_Entry **entries;
    int count;
};

Keyword_Table *make_keyword_table() {
    Keyword_Table *table = (Keyword_Table *)malloc(sizeof(Keyword_Table));
    table->count = 128;
    table->entries = (Keyword_Entry **)calloc(table->count, sizeof(Keyword_Entry * ));
    return table;
}

Keyword_Table *keyword_table;


Keyword_Entry *keyword_lookup(const char *str) {
    uint64 hash = hash_djb2((unsigned const char *)str, strlen(str));
    uint64 index = hash % keyword_table->count;

    Keyword_Entry *first = keyword_table->entries[index];
    for (Keyword_Entry *it = first; it; it = it->next) {
        if (strcmp(it->key, str) == 0) {
            return it;
        }
    }
    return nullptr;
}

void keyword_insert(const char *keyword) {
    Keyword_Entry *entry = (Keyword_Entry *)malloc(sizeof(Keyword_Entry));
    entry->key = keyword;
    for (Token_Type token = (Token_Type)((int)Token_KeywordFirst + 1); token < Token_KeywordLast; token++) {
        if (strcmp(token_type_to_string(token), keyword) == 0) {
            entry->token = token;
        }
    }

    uint64 hash = hash_djb2((unsigned const char *)keyword, strlen(keyword));
    uint64 index = hash % keyword_table->count;
    Keyword_Entry *first = keyword_table->entries[index];
    entry->next = first;
    keyword_table->entries[index] = entry;
    //printf("  entry %s: '%s' #%lld %lld\n", token_type_to_string(entry->token), keyword, hash, index);
}

void init_keywords() {
    keyword_table = make_keyword_table();

    //printf("initializing keyword table...\n");
    for (Token_Type token = (Token_Type)((int)Token_KeywordFirst + 1); token < Token_KeywordLast; token++) {
        const char *keyword = token_type_to_string(token);
        keyword_insert(keyword);
    }
}

void Lexer::error(const char *fmt, ...) {
    printf("%s(%d,%d) syntax error: ", source_name, l1, c1);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");

    error_count += 1;
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
    Token tok{};
begin:
    char *start = stream;
    int line_number = l1;
    int column_number = c1;

#define TOK1(C0, T0) \
    case C0: \
        advance(); \
        tok.type = T0; \
        break; \

#define TOK2(C0, T0, C1, T1) \
    case C0: \
        advance(); \
        tok.type = T0; \
        if (*stream == C1) { \
            advance(); \
            tok.type = T1; \
        } \
        break; \

#define TOK3(C0, T0, C1, T1, C2, T2) \
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
        TOK1(';', Token_Semicolon);
        TOK1(',', Token_Comma);

        TOK1('(', Token_OpenParen);
        TOK1(')', Token_CloseParen);
        TOK1('[', Token_OpenBracket);
        TOK1(']', Token_CloseBracket);
        TOK1('{', Token_OpenBrace);
        TOK1('}', Token_CloseBrace);

        TOK1('?', Token_Question);

        TOK2('+', Token_Plus,    '=', Token_AddAssign);
        TOK3('-', Token_Minus,   '=', Token_SubAssign, '>', Token_Arrow);
        TOK2('*', Token_Star,    '=', Token_MulAssign);
        TOK2('%', Token_Percent, '=', Token_ModAssign);
        TOK2('^', Token_Xor,     '=', Token_XorAssign);
        TOK2('~', Token_Tilde,   '=', Token_NotAssign);

        TOK2('!', Token_Bang, '=', Token_Neq);
        TOK2('=', Token_Assign, '=', Token_Equal);

        TOK3(':', Token_Colon, ':', Token_Colon2, '=', Token_ColonAssign);

    case '#':
    {
        //@Note Parse directives
        advance();
        while (isalnum(*stream) || *stream == '_') {
            advance();
        }
        int len = (int)(stream - start);
        Atom *atom = make_atom(start, len);
        Keyword_Entry *key = keyword_lookup(atom->name);
        if (key) {
            tok.type = key->token;
        } else {
            error("invalid directive '%s'", atom->name);
        }
        break;
    }

    case '.':
        advance();
        if (isdigit(*stream)) {
            stream = start;
            tok.type = Token_Floatlit;
            tok.floatlit = scan_float();
        } else if (*stream == '.') {
            advance();
            tok.type = Token_Ellipsis;
        } else {
            tok.type = Token_Dot;
        }
        break;

    case '/':
        advance();
        tok.type = Token_Slash;
        if (*stream == '=') {
            advance();
            tok.type = Token_DivAssign;
        } else if (*stream == '/') {
            eat_line();
            goto begin;
        }
        break;

    case '&':
        advance();
        tok.type = Token_Amper;
        if (*stream == '&') {
            advance();
            tok.type = Token_And;
        } else if (*stream == '=') {
            advance();
            tok.type = Token_AndAssign;
        }
        break;
    case '|':
        advance();
        tok.type = Token_Bar;
        if (*stream == '|') {
            advance();
            tok.type = Token_Or;
        } else if (*stream == '=') {
            advance();
            tok.type = Token_OrAssign;
        }
        break;
    case '<':
        advance();
        tok.type = Token_Lt;
        if (*stream == '=') {
            advance();
            tok.type = Token_Lteq;
        } else if (*stream == '<') {
            advance();
            tok.type = Token_Lshift;
        }
        break;
    case '>':
        advance();
        tok.type = Token_Gt;
        if (*stream == '=') {
            advance();
            tok.type = Token_Gteq;
        } else if (*stream == '>') {
            advance();
            tok.type = Token_Rshift;
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

        int len = (int)(stream - start) - 2;
        tok.type = Token_Strlit;
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
        int len = (int)(stream - start);
        Atom *atom = make_atom(start, len);

        Keyword_Entry *key = keyword_lookup(atom->name);
        if (key) {
            tok.type = key->token;
        } else {
            tok.type = Token_Ident;
            tok.name = atom;
        }
        break;
    }
    
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
    {
        while (isalnum(*stream)) {
            advance();
        }
        //@Note Fixes ellipsis conflict
        if (*stream == '.' && *(stream + 1) != '.') {
            stream = start;
            tok.type = Token_Floatlit;
            tok.floatlit = scan_float();
        } else {
            stream = start;
            tok.type = Token_Intlit;
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
        tok.type = Token_EOF;
        break;
    }
#undef TOK1
#undef TOK2
#undef TOK3

    tok.start = {line_number, column_number};

    l0 = line_number;
    c0 = column_number;

    // printf("%s (%d,%d)->(%d,%d)\n", token_type_to_string(tok.type), l0, c0, l1, c1);

    return tok;
}
