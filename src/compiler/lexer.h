#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "source_file.h"
#include "constant_value.h"

#define LEXER_MAX_STRING_LENGTH 1024

struct Atom;
struct Report;
struct Parser;

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
