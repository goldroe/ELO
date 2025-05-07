
global Token poisoned_token = {TOKEN_ERR};

internal void compiler_error(char *fmt, ...);
internal void add_source_file(Source_File *file);

internal Source_Pos source_pos(u64 line, u64 col, u64 index, Source_File *file) {
    Source_Pos result;
    result.line = line;
    result.col = col;
    result.index = index;
    result.file = file;
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


Lexer::Lexer(String8 file_path) {
    String8 file_text = {};
    OS_Handle file_handle = os_open_file(file_path, OS_AccessFlag_Read);
    if (os_valid_handle(file_handle)) {
        file_text = os_read_file_string(file_handle);
        os_close_handle(file_handle);

        source_file = new Source_File(file_path, file_text);
        add_source_file(source_file);
        stream = source_file->text.data;
        next_token();
    }
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
        result = digit - 'A';
    } else if (digit >= '0'  && digit <= '9') {
        result = digit - '0';
    } else {
        result = -1;
    }

    return result;
}

String8 Lexer::scan_number_suffix() {
    u8 *start = stream;
    for (;;) {
        u8 c = peek_character();
        if (!isalnum(c)) {
            break;
        }
        eat_char();
    }
    String8 result = str8(start, stream - start);
    return result;
}

Token Lexer::scan_integer() {
    Token result = {};
    int base = 10;

    u8 *start = stream;

    if (peek_character() == '0') {
        eat_char();
        u8 c = peek_character();
        if (c == 'X' || c == 'x') {
            eat_char();
            base = 16;
        } else if (c == 'b' || c == 'B') {
            eat_char();
            base = 2;
        }
    }

    bool bad_suffix = false;
    String8 suffix = str8_zero();

    u64 value = 0;
    while (isalnum(*stream)) {
        u8 digit_char = peek_character();
        int digit = get_next_hex_digit();

        if (digit >= base) {
            bad_suffix = true;
            continue;
        }

        if (digit == -1) {
            suffix = scan_number_suffix();
            break;
        }

        value = value * base + digit;
        eat_char();
    }

    if (bad_suffix) {
        report_parser_error(this, "bad suffix in number.\n");
        return poisoned_token;
    }

    String8 valid_suffixes[] = {
        str8_lit("s8"), 
        str8_lit("s16"), 
        str8_lit("s32"), 
        str8_lit("s64"), 
        str8_lit("u8"), 
        str8_lit("u16"), 
        str8_lit("u32"), 
        str8_lit("u64"), 
        str8_lit("f32"),
        str8_lit("f64"),
    };

    //@Note Encountered suffix to check
    if (suffix.count > 0) {
        bool illegal_suffix = false;

        if (!(suffix.count == 2 || suffix.count == 3)) {
            illegal_suffix = true;
        }

        String8 *valid_suffix = NULL;
        for (int i = 0; i < ArrayCount(valid_suffixes); i++) {
            String8 valid = valid_suffixes[i];
            if (valid.count == suffix.count) {
                bool equal = true;
                for (int j = 0; j < suffix.count; j++) {
                    if (tolower(suffix.data[j]) != valid.data[j]) {
                        equal = false;
                    }
                }

                if (equal) {
                    valid_suffix = &valid_suffixes[i];
                    break;
                }
            }
        }

        if (!valid_suffix) {
            illegal_suffix = true;
        }

        if (illegal_suffix) {
            report_parser_error(this, "illegal suffix: '%.*s'.\n", (int)suffix.count, suffix.data);
            return poisoned_token;
        }
    }

    result.kind = TOKEN_INTLIT;
    result.intlit = value;
    return result;
}

f64 Lexer::scan_float() {
    char *end_ptr = (char *)stream;
    f64 result = strtod((char *)stream, &end_ptr);
    for (char *ptr = (char *)stream; ptr < end_ptr; ptr++) {
        eat_char();
    }
    return result;
}

void Lexer::next_token() {
lex_start:
    u8 *begin = (u8 *)stream;

    Token token = {};
    token.start = source_pos(line_number, column_number, stream_index, source_file);
    
    switch (*stream) {
    default:
        token.kind = TOKEN_ERR;
        report_parser_error(this, "unknown character '%#x'.\n", *stream);
        eat_char();
        break;
        
    case '\0':
        token.kind = TOKEN_EOF;
        break;
        
    case ' ': case '\t': case '\f': case '\n': case '\r':
    {
        while (isspace(*stream)) {
            eat_char();
        }
        goto lex_start;
        break;
    }

    case '#':
    {
        eat_char();
        u8 *start = stream;
        while (isalpha(*stream) || *stream == '_') {
            u8 c = *stream;
            eat_char();
        }
        u64 count = stream - start;
        String8 string = str8(start, count);
        Atom *atom = atom_lookup(string);
        Assert(atom != NULL);
        break;
    }

    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '_': {
        while (isalnum(*stream) || *stream == '_') {
            u8 c = *stream;
            eat_char();
        }
        u64 count = stream - begin;
        String8 string = str8(begin, count);

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

    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
    {
        while (isalnum(*stream)) {
            eat_char();
        }
        bool do_float = false;
        if (peek_character() == '.') {
            if (peek_next_character() != '.') { //@Note Avoids conflict with ellipsis '..'
                do_float = true;
            }
        }

        //@Note Rewind
        stream_index = token.start.index;
        stream = source_file->text.data + stream_index;
        line_number = token.start.line;
        column_number = token.start.col;

        if (do_float) {
            f64 float_val = scan_float();
            token.kind = TOKEN_FLOATLIT;
            token.floatlit = float_val;
        } else {
            Token int_token = scan_integer();
            token.kind = TOKEN_INTLIT;
            token.intlit = int_token.intlit;
        }
        break;
    }

    case '"':
    {
        eat_char();
        u8 *start = stream;
        while (*stream && *stream != '"') {
            eat_char();
        }
        u64 count = stream - start;
        eat_char();

        String8 string = str8(start, count);
        token.kind = TOKEN_STRLIT;
        token.strlit = string; 
        break;
    }

    case '%':
    {
        token.kind = TOKEN_MOD;
        eat_char();
        if (*stream == '=') {
            token.kind = TOKEN_MOD_EQ;
            eat_char();
        }
        break;
    }

    case '&':
    {
        token.kind = TOKEN_AMPER;
        eat_char();
        if (*stream == '&') {
            token.kind = TOKEN_AND;
            eat_char();
        }
        break;
    }

    case '|':
    {
        token.kind = TOKEN_BAR;
        eat_char();
        if (*stream == '|') {
            token.kind = TOKEN_OR;
            eat_char();
        }
        break;
    }

    case '^':
    {
        token.kind = TOKEN_XOR;
        eat_char();

        if (*stream == '=') {
            token.kind = TOKEN_XOR_EQ;
            eat_char();
        }
        break;
    }

    case '!':
    {
        token.kind = TOKEN_BANG;
        eat_char();
        if (*stream == '=') {
            token.kind = TOKEN_NEQ;
            eat_char();
        }
        break;
    }

    case '+':
    {
        token.kind = TOKEN_PLUS;

        eat_char();
        if (*stream == '=') {
            eat_char();
            token.kind = TOKEN_PLUS_EQ;
        }
        break;
    }

    case '-':
    {
        token.kind = TOKEN_MINUS;

        eat_char();
        if (*stream == '>') {
            token.kind = TOKEN_ARROW;
            eat_char();
        } else if (*stream == '=') {
            token.kind = TOKEN_MINUS_EQ;
            eat_char();
        }
        break;
    }

    case '*':
    {
        token.kind = TOKEN_STAR;

        eat_char();
        if (*stream == '=') {
            token.kind = TOKEN_STAR_EQ;
            eat_char();
        }
        break;
    }

    case '/':
    {
        token.kind = TOKEN_SLASH;

        eat_char();
        if (*stream == '=') {
            token.kind = TOKEN_SLASH_EQ;
            eat_char();
        } else if (*stream == '/') {
            eat_char();
            eat_line();
            goto lex_start;
        }
        break;
    }

    case '=':
    {
        token.kind = TOKEN_EQ;

        eat_char();
        if (*stream == '=') {
            token.kind = TOKEN_EQ2;
            eat_char();
        }
        break;
    }

    case '<':
    {
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
    }

    case '>':
    {
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
    }

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
    {
        token.kind = TOKEN_DOT;
        eat_char();
        if (*stream == '.') {
            token.kind = TOKEN_ELLIPSIS;
            eat_char();
        } else if (*stream == '*') {
            token.kind = TOKEN_POSTFIX_DEREF;
            eat_char();
        }
        break; 
    }

    case ',':
    {
        token.kind = TOKEN_COMMA;
        eat_char();
        break;
    }

    case ';':
        token.kind = TOKEN_SEMI;
        eat_char();
        break;

    case ':':
    {
        token.kind = TOKEN_COLON;
        eat_char();
        if (*stream == '=') {
            token.kind = TOKEN_COLON_EQ;
            eat_char();
        } else if (*stream == ':') {
            token.kind = TOKEN_COLON2;
            eat_char();
        }
        break;
    }
    }

    token.end = source_pos(line_number, column_number, stream_index, source_file);

    current_token = token;
}

internal char *string_from_token(Token_Kind token) {
    switch (token) {
    default:
        return "UNKNOWN";

    case TOKEN_EOF:
        return "EOF";

    case TOKEN_IDENT:
        return "name";
    case TOKEN_INTLIT:
        return "intlit";
    case TOKEN_STRLIT:
        return "strlit";
    case TOKEN_FLOATLIT:
        return "floatlit";

    case TOKEN_SEMI:
        return ";";
    case TOKEN_COLON:
        return ":";
    case TOKEN_COLON2:
        return "::";
    case TOKEN_COMMA:
        return ",";
    case TOKEN_DOT:
        return ".";
    case TOKEN_ELLIPSIS:
        return "..";
    case TOKEN_ARROW:
        return "->";

    case TOKEN_PLUS:
        return "+";
    case TOKEN_MINUS:
        return "-";
    case TOKEN_STAR:
        return "*";
    case TOKEN_SLASH:
        return "/";
    case TOKEN_MOD:
        return "%";

    case TOKEN_COLON_EQ:
        return ":=";
    case TOKEN_PLUS_EQ:
        return "+=";
    case TOKEN_MINUS_EQ:
        return "-=";
    case TOKEN_STAR_EQ:
        return "*=";
    case TOKEN_SLASH_EQ:
        return "/=";
    case TOKEN_MOD_EQ:
        return "%=";
    case TOKEN_XOR_EQ:
        return "^=";
    case TOKEN_BAR_EQ:
        return "|=";
    case TOKEN_AMPER_EQ:
        return "&=";
    case TOKEN_LSHIFT_EQ:
        return "<<=";
    case TOKEN_RSHIFT_EQ:
        return ">>=";

    case TOKEN_EQ:
        return "=";
    case TOKEN_EQ2:
        return "==";
    case TOKEN_NEQ:
        return "!=";
    case TOKEN_LT:
        return "<";
    case TOKEN_LTEQ:
        return "<=";
    case TOKEN_GT:
        return ">";
    case TOKEN_GTEQ:
        return ">=";

    case TOKEN_BANG:
        return "!";
    case TOKEN_AMPER:
        return "&";
    case TOKEN_BAR:
        return "|";
    case TOKEN_AND:
        return "&&";
    case TOKEN_OR:
        return "||";
    case TOKEN_XOR:
        return "^";

    case TOKEN_POSTFIX_DEREF:
        return ".*";

    case TOKEN_LSHIFT:
        return "<<";
    case TOKEN_RSHIFT:
        return ">>";

    case TOKEN_LPAREN:
        return "(";
    case TOKEN_RPAREN:
        return ")";
    case TOKEN_LBRACKET:
        return "[";
    case TOKEN_RBRACKET:
        return "]";
    case TOKEN_LBRACE:
        return "{";
    case TOKEN_RBRACE:
        return "}";

    case TOKEN_NULL:
        return "null";
    case TOKEN_IF:
        return "if";
    case TOKEN_ELSE:
        return "else";
    case TOKEN_WHILE:
        return "while";
    case TOKEN_FOR:
        return "for";
    case TOKEN_RETURN:
        return "return";
    case TOKEN_CONTINUE:
        return "continue";
    case TOKEN_BREAK:
        return "break";
    case TOKEN_STRUCT:
        return "struct";
    case TOKEN_ENUM:
        return "enum";
    case TOKEN_TRUE:
        return "true";
    case TOKEN_FALSE:
        return "false";
    }
}
