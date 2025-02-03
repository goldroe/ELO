#ifndef LEXER_H
#define LEXER_H

struct Source_Pos {
    u64 col;
    u64 line;
    u64 index;
};

struct Atom;

enum Token_Kind {
    TOKEN_ERR = -1,
    TOKEN_EOF,

    //@Note Literals
    TOKEN_IDENT,
    TOKEN_INTLIT,
    TOKEN_STRLIT,
    TOKEN_FLOATLIT,

    TOKEN_SEMI,
    TOKEN_COLON,
    TOKEN_COLON2,
    TOKEN_COMMA,
    TOKEN_ARROW,

    //@Note Operators
    TOKEN_OPERATOR_BEGIN,

    TOKEN_DOT,
    TOKEN_ELLIPSIS,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_MOD,

    TOKEN_COLON_EQ,

    //@Note Assignment operators
    TOKEN_ASSIGN_BEGIN,
    TOKEN_EQ,
    TOKEN_PLUS_EQ,
    TOKEN_MINUS_EQ,
    TOKEN_STAR_EQ,
    TOKEN_SLASH_EQ,
    TOKEN_MOD_EQ,
    TOKEN_XOR_EQ,
    TOKEN_BAR_EQ,
    TOKEN_AMPER_EQ,
    TOKEN_LSHIFT_EQ,
    TOKEN_RSHIFT_EQ,
    TOKEN_ASSIGN_END,

    TOKEN_EQ2,
    TOKEN_NEQ,
    TOKEN_LT,
    TOKEN_LTEQ,
    TOKEN_GT,
    TOKEN_GTEQ,

    TOKEN_BANG,
    TOKEN_AMPER,
    TOKEN_BAR,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_XOR,

    TOKEN_LSHIFT,
    TOKEN_RSHIFT,

    TOKEN_LBRACKET,
    TOKEN_RBRACKET,

    TOKEN_OPERATOR_END,

    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,

    //@Note Keywords
    TOKEN_KEYWORD_BEGIN,
    TOKEN_NULL,
    TOKEN_ENUM,
    TOKEN_STRUCT,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_RETURN,
    TOKEN_CAST,
    TOKEN_OPERATOR,
    TOKEN_IN,
    TOKEN_KEYWORD_END,

    //@Note Directives
    TOKEN_DIRECTIVE_BEGIN,
    TOKEN_LOAD,
    TOKEN_IMPORT,
    TOKEN_DIRECTIVE_END,

    Token_COUNT
};

struct Token {
    Token_Kind kind;
    Source_Pos start, end;

    union {
        Atom *name;
        String8 strlit;
        u64 intlit;
        f64 floatlit;
    };
};

struct Lexer {
    String8 file_path;
    String8 file_contents;

    u8 *stream = NULL;
    u64 line_number = 1;
    u64 column_number = 0;
    u64 stream_index = 0;

    Token current_token;

    int error_count = 0;

    Lexer(String8 file_name);

    void error(const char *fmt, ...);

    bool match(Token_Kind token);
    bool eof();

    Token lookahead(int n);
    void rewind(Token token);

    bool eat(Token_Kind token);
    Token_Kind peek();
    Token current();

    int get_next_hex_digit();

    u64 scan_integer();
    f64 scan_float();

    u8 peek_character();
    u8 peek_next_character();
    void eat_char();
    void eat_line();
    void next_token();
};

#endif // LEXER_H
