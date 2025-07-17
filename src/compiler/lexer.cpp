
global Token poisoned_token = {TOKEN_UNKNOWN};

global u8 g_lexer_string_buffer[LEXER_MAX_STRING_LENGTH];

internal void compiler_error(char *fmt, ...);

internal const char *string_from_token(Token_Kind token) {
    return token_strings[token];
}

u8 Lexer::get_escape_character(u8 c) {
    switch (c) {
    default:
        report_parser_error(this, "illegal escape character in string, '%c'.\n", c);
        return 0;
        break;
    case '\\': return '\\';
    case '0':  return 0;
    case 't':  return '\t';
    case 'f': return '\f';
    case 'v': return '\v';
    case 'n':  return '\n';
    case 'r':  return '\r';
    case '\'': return '\'';
    }
}

internal Source_Pos source_pos_make(Source_File *file, u64 line, u64 col, u64 index) {
    Source_Pos result;
    result.file = file;
    result.line = line;
    result.col = col;
    result.index = index;
    return result;
}

internal inline bool is_assignment_op(Token_Kind op) {
    return TOKEN_ASSIGN_BEGIN < op && op < TOKEN_ASSIGN_END;
}

internal bool is_unary_op(Token_Kind token) {
    switch (token) {
    default:
        return false;
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_BANG:
    case TOKEN_STAR:
    case TOKEN_XOR:
        return true;
    }
}

internal bool is_operator(Token_Kind op) {
    return TOKEN_OPERATOR_BEGIN < op && op < TOKEN_OPERATOR_END;
}

internal bool operator_is_overloadable(Token_Kind op) {
    switch (op) {
    default:
        return false;
        
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_MOD:
    case TOKEN_PLUS_EQ:
    case TOKEN_MINUS_EQ:
    case TOKEN_STAR_EQ:
    case TOKEN_SLASH_EQ:
    case TOKEN_MOD_EQ:
    case TOKEN_XOR_EQ:
    case TOKEN_BAR_EQ:
    case TOKEN_AMPER_EQ:
    case TOKEN_LSHIFT_EQ:
    case TOKEN_RSHIFT_EQ:
    case TOKEN_EQ2:
    case TOKEN_NEQ:
    case TOKEN_LT:
    case TOKEN_LTEQ:
    case TOKEN_GT:
    case TOKEN_GTEQ:
    case TOKEN_BANG:
    case TOKEN_BAR:
    case TOKEN_AMPER:
    case TOKEN_AND:
    case TOKEN_OR:
    case TOKEN_XOR:
    case TOKEN_LSHIFT:
    case TOKEN_RSHIFT:
    case TOKEN_LBRACKET:
    case TOKEN_RBRACKET:
        return true;
    }
}

Lexer::Lexer(Source_File *source) {
    set_source_file(source);
}

void Lexer::set_source_file(Source_File *source) {
    source_file = source;
    stream = source->text.data;
    line_number = 1;
    column_number = 0;
    stream_index = 0;
    next_token();
}

Token Lexer::current() {
    return current_token;
}

Token_Kind Lexer::peek() {
    return current_token.kind;
}

u8 Lexer::peek_character() {
    return *stream;
}

u8 Lexer::peek_next_character() {
    u8 result = 0;
    if (*stream != 0) {
        result = *(stream + 1);
    }
    return result;
} 

bool Lexer::eof() {
    return peek() == TOKEN_EOF;
}

bool Lexer::match(Token_Kind token) {
    Token_Kind t = peek();
    return t == token;
}

bool Lexer::eat(Token_Kind token) {
    if (match(token)) {
        next_token();
        return true;
    }
    return false;
}

void Lexer::eat_char() {
    u8 c = *stream;
    stream++;
    stream_index++;
    if (c == '\n') {
        line_number++;
        column_number = 0;
    } else if (c == '\r') {
        if (*stream == '\n') {
            stream++;
            stream_index++;
        }
        line_number++;
        column_number = 0;
    } else {
        column_number++;
    }
}

void Lexer::eat_line() {
    while (*stream) {
        if (*stream == '\r') {
            eat_char();
            if (*stream == '\n') {
                eat_char();
            }
            break;
        } else if (*stream == '\n') {
            eat_char();
            break;
        }

        eat_char();
    }
}

void Lexer::rewind(Token token) {
    stream_index = token.end.index;
    stream = source_file->text.data + stream_index;
    line_number = token.end.line;
    column_number = token.end.col;
    current_token = token;
}

Token Lexer::lookahead(int n) {
    Assert(n >= 0);
    Token curr = current();

    for (int i = 0; i < n; i++) {
        next_token();
    }
    Token result = current();
    rewind(curr);
    return result;
}

int Lexer::get_next_hex_digit() {
    int result = 0;

    u8 digit = peek_character();
    digit = (u8)toupper(digit);

    if (digit >= 'A' && digit <= 'F') {
        result = digit - 'A' + 10;
    } else if (digit >= '0'  && digit <= '9') {
        result = digit - '0';
    } else {
        result = -1;
    }

    return result;
}

String Lexer::scan_number_suffix() {
    u8 *start = stream;
    for (;;) {
        u8 c = peek_character();
        if (!isalnum(c)) {
            break;
        }
        eat_char();
    }
    String result = str8(start, stream - start);
    return result;
}

void Lexer::eat_comment_region() {
    while (comment_region_level != 0) {
        if (*stream == 0) {
            report_parser_error(this, "unexpected EOF in comment region.\n");
            return;
        }

        if (*stream == '*') {
            eat_char();
            if (*stream == '/') {
                eat_char();
                comment_region_level--;
            }
        } else if (*stream == '/') {
            eat_char();
            if (*stream == '*') {
                eat_char();
                comment_region_level++;
            }
        } else {
            eat_char();
        }
    }
}

f64 Lexer::scan_mantissa() {
    char *start = (char *)stream;
    eat_char();
    while (isdigit(peek_character())) {
        eat_char();
    }

    f64 mantissa = strtod(start, nullptr);
    return mantissa;
}

void Lexer::scan_number(Token *token) {
    int base = 10;

    if (peek_character() == '0') {
        eat_char();
        u8 c = peek_character();
        if (c == 'X' || c == 'x') {
            eat_char();
            base = 16;
        } else if (c == 'b' || c == 'B') {
            eat_char();
            base = 2;
        } else if (isalnum(c)) {
            base = 8;
        }
    }

    bigint integer = {};
    bool is_floating = false;
    f64 mantissa = 0.0;

    for (;;) {
        if (peek_character() == '.' && peek_next_character() != '.') {  //@Note Avoids conflict with ellipsis '..'
            is_floating = true;
            mantissa = scan_mantissa();
            break;
        }

        int digit = get_next_hex_digit();
        if (digit >= base) {
            break;
        }
        if (digit == -1) {
            break;
        }

        // N = N * B + D
        mp_mul_d(&integer, base, &integer);
        mp_add_d(&integer, digit, &integer);

        eat_char();
    }

    String suffix = scan_number_suffix();

    token->literal_kind = LITERAL_DEFAULT;

    //@Note Encountered suffix to check
    if (suffix.count > 0) {
        Suffix_Literal *valid_suffix = nullptr;
        for (int i = 0; i < ArrayCount(suffix_literals); i++) {
            Suffix_Literal suffix_literal = suffix_literals[i];
            if (str8_match(suffix_literal.string, suffix, StringMatchFlag_CaseInsensitive)) {
                valid_suffix = &suffix_literals[i]; 
                break;
            }
        }

        if (valid_suffix) {
            token->literal_kind = valid_suffix->literal;
        } else {
            report_parser_error(this, "illegal literal suffix: '%S'.\n", suffix);
        }
    }

    if (is_floating) {
        f64 f = f64_from_bigint(integer) + mantissa;
        token->kind = TOKEN_FLOAT;
        token->value = constant_value_float_make(f);
        if (LITERAL_U8 <= token->literal_kind && token->literal_kind <= LITERAL_I64) {
            report_parser_error(this, "illegal integer suffix on floating-point.\n");
        }
    } else {
        token->kind = TOKEN_INTEGER;
        token->value = constant_value_int_make(integer);
        if (token->literal_kind == LITERAL_F32 || token->literal_kind == LITERAL_F64) {
            report_parser_error(this, "illegal floating-point suffix on integer.\n");
        }
    }
}

void Lexer::next_token() {
lex_start:
    u8 *begin = (u8 *)stream;

    Token token = {};
    token.start = source_pos_make(source_file, line_number, column_number, stream_index);
    
    switch (*stream) {
    default:
        token.kind = TOKEN_UNKNOWN;
        report_parser_error(this, "unknown character '%#x'.\n", *stream);
        eat_char();
        break;
        
    case '\0':
        token.kind = TOKEN_EOF;
        break;
        
    case ' ': case '\t': case '\f': case '\n': case '\r':
        while (isspace(*stream)) {
            eat_char();
        }
        goto lex_start;

    case '#':
        token.kind = TOKEN_HASH;
        eat_char();
        break;

    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '_': {
        while (isalnum(*stream) || *stream == '_') {
            u8 c = *stream;
            eat_char();
        }
        u64 count = stream - begin;
        String string = str8(begin, count);

        Atom *atom = atom_create(string);
        if (atom->flags & ATOM_FLAG_IDENT) {
            token.kind = TOKEN_IDENT;
            token.name = atom;
        } else if (atom->flags & ATOM_FLAG_KEYWORD) {
            token.kind = atom->token;
        } else {
            Assert(0);
        }
        break;
    }

    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
        scan_number(&token);
        break;
    }

    case '"': {
        token.kind = TOKEN_STRING;

        eat_char();
        u8 *start = stream;
        int count = 0;
        while (*stream && *stream != '"') {
            if (count >= LEXER_MAX_STRING_LENGTH)  {
                report_parser_error(this, "string literal achieved max capacity.\n");
                break;
            }

            u8 character = peek_character();
            eat_char();
            if (character == '\\') {
                u8 escape = peek_character();
                eat_char();
                character = get_escape_character(escape);
            }
            g_lexer_string_buffer[count] = character;
            count++;
        }
        eat_char(); // eat '"'

        g_lexer_string_buffer[count] = 0;
        String string = str8_copy(heap_allocator(), str8(g_lexer_string_buffer, count));
        token.value = constant_value_string_make(string);
        break;
    }

    case '%':
        token.kind = TOKEN_MOD;
        eat_char();
        if (*stream == '=') {
            token.kind = TOKEN_MOD_EQ;
            eat_char();
        }
        break;

    case '&':
        token.kind = TOKEN_AMPER;
        eat_char();
        if (*stream == '&') {
            token.kind = TOKEN_AND;
            eat_char();
        }
        break;

    case '|':
        token.kind = TOKEN_BAR;
        eat_char();
        if (*stream == '|') {
            token.kind = TOKEN_OR;
            eat_char();
        }
        break;

    case '^':
        token.kind = TOKEN_XOR;
        eat_char();
        if (*stream == '=') {
            token.kind = TOKEN_XOR_EQ;
            eat_char();
        }
        break;

    case '~':
        token.kind = TOKEN_SQUIGGLE;
        eat_char();
        break;

    case '!':
        token.kind = TOKEN_BANG;
        eat_char();
        if (*stream == '=') {
            token.kind = TOKEN_NEQ;
            eat_char();
        }
        break;

    case '+':
        token.kind = TOKEN_PLUS;
        eat_char();
        if (*stream == '=') {
            eat_char();
            token.kind = TOKEN_PLUS_EQ;
        } else if (*stream == '+') {
            eat_char();
            token.kind = TOKEN_INCREMENT;
        }
        break;

    case '-':
        token.kind = TOKEN_MINUS;
        eat_char();
        if (*stream == '>') {
            token.kind = TOKEN_ARROW;
            eat_char();
        } else if (*stream == '=') {
            token.kind = TOKEN_MINUS_EQ;
            eat_char();
        } else if (*stream == '-') {
            token.kind = TOKEN_DECREMENT;
            eat_char();
            if (*stream == '-') {
                token.kind = TOKEN_UNINIT;
                eat_char();
            }
        }
        break;

    case '*':
        token.kind = TOKEN_STAR;
        eat_char();
        if (*stream == '=') {
            token.kind = TOKEN_STAR_EQ;
            eat_char();
        }
        break;

    case '/':
        token.kind = TOKEN_SLASH;
        eat_char();
        if (*stream == '=') {
            token.kind = TOKEN_SLASH_EQ;
            eat_char();
        } else if (*stream == '/') {
            eat_line();
            goto lex_start;
        } else if (*stream == '*') {
            eat_char();
            comment_region_level = 1;
            eat_comment_region();
            goto lex_start;
        }
        break;

    case '=':
        token.kind = TOKEN_EQ;
        eat_char();
        if (*stream == '=') {
            token.kind = TOKEN_EQ2;
            eat_char();
        }
        break;

    case '<':
        token.kind = TOKEN_LT;
        eat_char();
        if (*stream == '<') {
            token.kind = TOKEN_LSHIFT;
            eat_char();
            if (*stream == '=') {
                token.kind = TOKEN_LSHIFT_EQ;
                eat_char();
            }
        } else if (*stream == '=') {
            token.kind = TOKEN_LTEQ;
            eat_char();
        }
        break;

    case '>':
        token.kind = TOKEN_GT;
        eat_char();
        if (*stream == '>') {
            token.kind = TOKEN_RSHIFT;
            eat_char();
            if (*stream == '=') {
                token.kind = TOKEN_RSHIFT_EQ;
                eat_char();
            }
        } else if (*stream == '=') {
            token.kind = TOKEN_GTEQ;
            eat_char();
        }
        break;

    case '(':
        token.kind = TOKEN_LPAREN;
        eat_char();
        break;
    case ')':
        token.kind = TOKEN_RPAREN;
        eat_char();
        break;

    case '[':
        token.kind = TOKEN_LBRACKET;
        eat_char();
        break;
    case ']':
        token.kind = TOKEN_RBRACKET;
        eat_char();
        break;

    case '{':
        token.kind = TOKEN_LBRACE;
        eat_char();
        break;
    case '}':
        token.kind = TOKEN_RBRACE;
        eat_char();
        break;

    case '.':
        token.kind = TOKEN_DOT;
        eat_char();
        if (*stream == '.') {
            token.kind = TOKEN_ELLIPSIS;
            eat_char();
        } else if (*stream == '*') {
            token.kind = TOKEN_DOT_STAR;
            eat_char();
        }
        break; 

    case '\'': {
        eat_char();
        int c = 0;
        if (peek_character() == '\\') {
            eat_char();
            c = get_escape_character(peek_character());
            eat_char();
        } else {
            c = *stream;
            eat_char();
        }
        eat_char();

        token.kind = TOKEN_INTEGER;
        token.value = constant_value_int_make(bigint_make(c));
        token.literal_kind = LITERAL_U8;
        break;
    }

    case ',': {
        token.kind = TOKEN_COMMA;
        eat_char();
        break;
    }

    case ';':
        token.kind = TOKEN_SEMI;
        eat_char();
        break;

    case ':':
        token.kind = TOKEN_COLON;
        eat_char();
        break;
    }

    token.end = source_pos_make(source_file, line_number, column_number, stream_index);
    token.string = str8(begin, token.end.index - token.start.index);

    current_token = token;
}
