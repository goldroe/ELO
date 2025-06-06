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
    TOKEN_SQUIGGLE,

    TOKEN_LSHIFT,
    TOKEN_RSHIFT,

    TOKEN_LBRACKET,
    TOKEN_RBRACKET,

    TOKEN_DOT_STAR,

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
    TOKEN_IFCASE,
    TOKEN_CASE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_FALLTHROUGH,
    TOKEN_RETURN,
    TOKEN_CAST,
    TOKEN_OPERATOR,
    TOKEN_IN,
    TOKEN_SIZEOF,
    TOKEN_TYPEOF,
    TOKEN_KEYWORD_END,

    //@Note Directives
    TOKEN_DIRECTIVE_BEGIN,
    TOKEN_LOAD,
    TOKEN_IMPORT,
    TOKEN_FOREIGN,
    TOKEN_TYPEDEF,
    TOKEN_COMPLETE,
    TOKEN_DIRECTIVE_END,

    TOKEN_COUNT
};

enum Literal_Flags {
    LITERAL_NULL     = (1<<0),
    LITERAL_INT      = (1<<1),
    LITERAL_FLOAT    = (1<<2),
    LITERAL_BOOLEAN  = (1<<3),
    LITERAL_STRING   = (1<<4),
    LITERAL_U8       = (1<<5),
    LITERAL_U16      = (1<<6),
    LITERAL_U32      = (1<<7),
    LITERAL_U64      = (1<<8),
    LITERAL_I8       = (1<<9),
    LITERAL_I16      = (1<<10),
    LITERAL_I32      = (1<<11),
    LITERAL_I64      = (1<<12),
    LITERAL_F32      = (1<<13),
    LITERAL_F64      = (1<<14),
};

struct Token {
    Token_Kind kind = TOKEN_ERR;
    Source_Pos start, end;
    Literal_Flags literal_flags = (Literal_Flags)0;

    union {
        Atom *name;
        String8 strlit;
        u64 intlit;
        f64 floatlit;
    };
};

struct Lexer {
    Parser *parser;
    Source_File *source_file;

    u8 *stream = NULL;
    u64 line_number = 1;
    u64 column_number = 0;
    u64 stream_index = 0;

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

    bool is_valid_suffix(String8 suffix);
    String8 scan_number_suffix();
    Token scan_integer();
    f64 scan_float();

    u8 get_escape_character(u8 c);
    u8 peek_character();
    u8 peek_next_character();
    void eat_char();
    void eat_line();
    void next_token();
};


internal Source_Pos make_source_pos(Source_File *file, u64 line, u64 col, u64 index);

#endif // LEXER_H
