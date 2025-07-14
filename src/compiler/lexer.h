#ifndef LEXER_H
#define LEXER_H

#define LEXER_MAX_STRING_LENGTH 1024

struct Atom;
struct Report;
struct Parser;

struct Source_Pos {
    Source_File *file;
    u64 line;
    u64 col;
    u64 index;
};

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
    TOKEN_KIND(TOKEN_OPERATOR_BEGIN, "Operator__Begin"), \
    TOKEN_KIND(TOKEN_DOT, "."), \
    TOKEN_KIND(TOKEN_ELLIPSIS, ".."), \
    TOKEN_KIND(TOKEN_PLUS, "+"), \
    TOKEN_KIND(TOKEN_MINUS, "-"), \
    TOKEN_KIND(TOKEN_STAR, "*"), \
    TOKEN_KIND(TOKEN_SLASH, "/"), \
    TOKEN_KIND(TOKEN_MOD, "%"), \
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
    TOKEN_KIND(TOKEN_LPAREN, "("), \
    TOKEN_KIND(TOKEN_RPAREN, ")"), \
    TOKEN_KIND(TOKEN_LBRACE, "{"), \
    TOKEN_KIND(TOKEN_RBRACE, "}"), \
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
    TOKEN_KIND(TOKEN_DIRECTIVE_BEGIN, "Directive__Begin"), \
    TOKEN_KIND(TOKEN_LOAD, "#load"), \
    TOKEN_KIND(TOKEN_IMPORT, "#import"), \
    TOKEN_KIND(TOKEN_FOREIGN, "#foreign"), \
    TOKEN_KIND(TOKEN_TYPEDEF, "#type"), \
    TOKEN_KIND(TOKEN_COMPLETE, "#complete"), \
    TOKEN_KIND(TOKEN_DIRECTIVE_END, "Directive__End"), \

enum Token_Kind {
#define TOKEN_KIND(K,S) K
    TOKEN_KINDS
#undef TOKEN_KIND
    TOKEN_COUNT
};

const char *token_strings[] = {
#define TOKEN_KIND(K,S) S
    TOKEN_KINDS
#undef TOKEN_KIND
};

enum Literal_Kind {
    LITERAL_DEFAULT,
    LITERAL_U8,
    LITERAL_U16,
    LITERAL_U32,
    LITERAL_U64,
    LITERAL_I8,
    LITERAL_I16,
    LITERAL_I32,
    LITERAL_I64,
    LITERAL_F32,
    LITERAL_F64,
};

struct Token {
    Token_Kind kind = TOKEN_UNKNOWN;
    Source_Pos start = {};
    Source_Pos end = {};

    union {
        Atom *name;
        struct {
            Constant_Value value;
            Literal_Kind literal_kind;
        };
    };
};

struct Lexer {
    Parser *parser;
    Source_File *source_file;

    u8 *stream = NULL;
    u64 line_number = 1;
    u64 column_number = 0;
    u64 stream_index = 0;

    int comment_region_level = 0;
    Token current_token;

    int error_count = 0;

    Lexer(Source_File *source);

    void set_source_file(Source_File *source_file);

    bool match(Token_Kind token);
    bool eof();

    Token lookahead(int n);
    void rewind(Token token);

    bool eat(Token_Kind token);
    Token_Kind peek();
    Token current();

    int get_next_hex_digit();
    String8 scan_number_suffix();
    f64 Lexer::scan_mantissa();
    void Lexer::scan_number(Token *token);

    u8 get_escape_character(u8 c);
    u8 peek_character();
    u8 peek_next_character();
    void eat_char();
    void eat_line();
    void next_token();

    void eat_comment_region();
};


internal Source_Pos source_pos_make(Source_File *file, u64 line, u64 col, u64 index);

#endif // LEXER_H
