#define ANSI_RESET     "\x1B[0m"
#define ANSI_UNDERLINE "\x1B[4m"
#define ANSI_ITALIC    "\x1B[3m"

internal Token ast_start_token(Ast *node);
internal Token ast_end_token(Ast *node);

global Arena *g_report_arena;
global int g_error_count;

internal u64 get_line_start_after_spaces(String8 string, u64 start) {
    for (u64 i = start; i < string.count; i++) {
        if (string.data[i] == '\n' || string.data[i] == '\r') {
            return start;
        }
        if (!isspace(string.data[i])) {
            return i;
        }
    }
    return start;
}

internal u64 get_next_line_boundary(String8 string, u64 start) {
    for (u64 i = start; i < string.count; i++) {
        if (string.data[i] == '\n' || string.data[i] == '\r') {
            return i;
        }
    }
    return string.count - 1;
}

internal void report_out_va(const char *fmt, va_list args) {
    vfprintf(stderr, fmt, args);
}

internal void report_out(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    report_out_va(fmt, args);
    va_end(args);
}

internal void report_line(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    report_out_va(fmt, args);
    report_out("\n");
    va_end(args);
}

internal void report_error_line(Source_File *file, Source_Pos pos) {
    String8 buffer = file->text; 
    u64 line_pos = pos.index - pos.col;
    u64 line_end = get_next_line_boundary(buffer, pos.index);
    u64 len = line_end - line_pos;
    int before_len = (int)(pos.index - line_pos);
    String8 string = str8(buffer.data + line_pos, len);

    report_out("%.*s\n", (int)string.count, string.data);

    for (int i = 0; i < before_len; i++) {
        report_out(" ");
    }

    report_out("\x1B[38;2;204;36;29m");
    report_out("^");
    report_out(ANSI_RESET);

    report_out("\n");
}

internal void report_parser_error_va(Source_Pos pos, const char *fmt, va_list va) {
    report_out("\x1B[38;2;204;36;29m");
    report_out("syntax error: ");
    report_out(ANSI_RESET);

    report_out_va(fmt, va);

    if (pos.line != 0) {
        report_out("%.*s:%llu:%llu\n", (int)pos.file->path.count, pos.file->path.data, pos.line, pos.col);
        report_error_line(pos.file, pos);
    }
}

internal void report_parser_error(Token token, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    report_parser_error_va(token.start, fmt, args);
    va_end(args);
    g_error_count++;
}

internal void report_parser_error(Lexer *lexer, const char *fmt, ...) {
    Token token = lexer->current();
    va_list args;
    va_start(args, fmt);
    report_parser_error_va(token.start, fmt, args);
    va_end(args);
    g_error_count++;
}

internal void report_ast_error_va(Ast *node, const char *fmt, va_list va) {
    Source_File *file = nullptr;
    Token start_token = {};
    Token end_token = {};
    if (node) {
        file = node->file;
        start_token = ast_start_token(node);
        end_token = ast_end_token(node);
    }

    Source_Pos pos = start_token.start;
    Source_Pos end = end_token.end;


    report_out("\x1B[38;2;204;36;29m");
    report_out("error: ");
    report_out(ANSI_RESET);

    report_out_va(fmt, va);

    if (!node) {
        return;
    }

    report_out("%s:%llu:%llu:\n", file->path.data, pos.line, pos.col);

    String8 buffer = file->text; 
    u64 line_pos = pos.index - pos.col;
    u64 line_end = get_next_line_boundary(buffer, pos.index);
    int before_len = (int)(pos.index - line_pos);
    u64 end_index = Min(end.index, line_end);

    String8 string = str8(buffer.data + line_pos, line_end - line_pos);
    report_out("%.*s\n", (int)string.count, string.data);

    for (int i = 0; i < before_len; i++) {
        report_out(" ");
    }

    report_out("\x1B[38;2;204;36;29m");
    for (int i = 0; i < end_index - pos.index; i++) {
        report_out("^");
    }
    report_out(ANSI_RESET);

    report_out("\n");

    if (end.index > line_end) {
        report_out(ANSI_ITALIC "\x1B[38;2;168;153;132m");
        report_out("...");
        report_out(ANSI_RESET);
    }
}

internal void report_ast_error(Ast *node, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    report_ast_error_va(node, fmt, args);
    va_end(args);
    g_error_count++;
}

internal void report_note_va(Source_Pos pos, const char *fmt, va_list va) {
    report_out("%s:%llu:%llu: note: ", pos.file->path.data, pos.line, pos.col);
    report_out_va(fmt, va);
}

internal void report_note(Source_Pos pos, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    report_note_va(pos, fmt, args);
    va_end(args);
}

void report_undeclared(Ast_Ident *ident) {
    report_ast_error(ident, "undeclared identifier '%s'.\n", ident->name->data);
}

void report_redeclaration(Ast_Decl *decl) {
    report_ast_error(decl, "redeclaration of '%s'.\n", decl->name->data);
}

internal Token ast_start_token(Ast *node) {
    switch (node->kind) {
    default:
        Assert(0);
        break;
    case AST_BAD_DECL:
        break;
    case AST_PARAM: {
        ast_node_var(param, Ast_Param, node);
        if (param->name) {
            return ast_start_token(param->name);
        } else {
            return ast_start_token(param->typespec);
        }
    }

    case AST_ENUM_FIELD:
        return ast_start_token(ast_node(Ast_Enum_Field, node)->name);

    case AST_PROC_LIT:
        break;

    case AST_VALUE_DECL: {
        ast_node_var(vd, Ast_Value_Decl, node);
        if (vd->names.count) {
            return ast_start_token(vd->names[0]);
        } else {
            return ast_start_token(vd->values[0]);
        }
        break;
    }

    case AST_EMPTY_STMT:
        return ast_node(Ast_Empty_Stmt, node)->token;
    case AST_BAD_STMT:
        return ast_node(Ast_Bad_Stmt, node)->start;
    case AST_EXPR_STMT:
        return ast_start_token(ast_node(Ast_Expr_Stmt, node)->expr);
    case AST_ASSIGNMENT: {
        ast_node_var(assignment, Ast_Assignment, node);
        if (assignment->lhs.count) {
            return ast_start_token(assignment->lhs[0]);
        }
        return assignment->token;
    }
    case AST_IF:
        return ast_node(Ast_If, node)->token;
    case AST_IFCASE:
        return ast_node(Ast_Ifcase, node)->token;
    case AST_CASE_LABEL:
        return ast_node(Ast_Case_Label, node)->token;
    case AST_DO_WHILE:
        return ast_node(Ast_Do_While, node)->token;
    case AST_WHILE:
        return ast_node(Ast_While, node)->token;
    case AST_FOR:
        return ast_node(Ast_For, node)->token;
    case AST_BLOCK:
        return ast_node(Ast_Continue, node)->token;
    case AST_RETURN:
        return ast_node(Ast_Return, node)->token;
    case AST_CONTINUE:
        return ast_node(Ast_Continue, node)->token;
    case AST_BREAK:
        return ast_node(Ast_Break, node)->token;
    case AST_FALLTHROUGH:
        return ast_node(Ast_Fallthrough, node)->token;
    case AST_LOAD_STMT:
        return ast_node(Ast_Load, node)->token;
    case AST_IMPORT_STMT:
        return ast_node(Ast_Import, node)->token;
    case AST_DEFER_STMT:
        return ast_node(Ast_Defer, node)->token;

    case AST_BAD_EXPR:
        return ast_node(Ast_Bad_Expr, node)->start;
    case AST_PAREN:
        return ast_node(Ast_Paren, node)->open;
    case AST_LITERAL:
        return ast_node(Ast_Literal, node)->token;

    case AST_COMPOUND_LITERAL: {
        ast_node_var(compound, Ast_Compound_Literal, node);
        if (compound->type) {
            return ast_start_token(compound->typespec);
        }
        return compound->open;
        break;
    }
    case AST_IDENT:
        return ast_node(Ast_Ident, node)->token;
    case AST_CALL: {
        ast_node_var(call, Ast_Call, node);
        return ast_start_token(call->elem);
    }
    case AST_SUBSCRIPT:
        return ast_node(Ast_Subscript, node)->open;
    case AST_CAST:
        return ast_node(Ast_Cast, node)->token;
    case AST_ITERATOR:
        break;
    case AST_UNARY:
        return ast_node(Ast_Unary, node)->token;
    case AST_ADDRESS:
        return ast_node(Ast_Address, node)->token;
    case AST_DEREF: {
        ast_node_var(deref, Ast_Deref, node);
        return ast_start_token(deref->elem);
    }
    case AST_BINARY: {
        ast_node_var(binary, Ast_Binary, node);
        return ast_start_token(binary->rhs);
    }
    case AST_SELECTOR: {
        ast_node_var(selector, Ast_Selector, node);
        return ast_start_token(selector->parent);
    }
    case AST_RANGE: {
        ast_node_var(range, Ast_Range, node);
        return ast_start_token(range->rhs);
    }
    case AST_SIZEOF:
        return ast_node(Ast_Sizeof, node)->token;

    case AST_POINTER_TYPE: {
        ast_node_var(type, Ast_Pointer_Type, node);
        return ast_start_token(type->elem);
    }
    case AST_ARRAY_TYPE: {
        ast_node_var(type, Ast_Array_Type, node);
        return ast_start_token(type->elem);
    }
    case AST_PROC_TYPE:
        return ast_node(Ast_Proc_Type, node)->open;
    case AST_ENUM_TYPE:
        return ast_node(Ast_Enum_Type, node)->token;
    case AST_STRUCT_TYPE:
        return ast_node(Ast_Struct_Type, node)->token;
    case AST_UNION_TYPE:
        return ast_node(Ast_Union_Type, node)->token;
    }

    return {};
}

internal Token ast_end_token(Ast *node) {
    switch (node->kind) {
    default:
        Assert(0);
        break;
    case AST_BAD_DECL:
        break;
    case AST_PARAM: {
        ast_node_var(param, Ast_Param, node);
        if (param->typespec) {
            return ast_end_token(param->typespec);
        } else {
            return ast_end_token(param->name);
        }
    }

    case AST_ENUM_FIELD: {
        ast_node_var(field, Ast_Enum_Field, node);
        if (field->expr) {
            return ast_end_token(field->expr);
        }
        return ast_end_token(field->name);
    }

    case AST_PROC_LIT:
        break;

    case AST_VALUE_DECL: {
        ast_node_var(vd, Ast_Value_Decl, node);
        if (vd->values.count) {
            return ast_end_token(vd->values.back());
        } else {
            return ast_end_token(vd->typespec);
        }
        break;
    }

    case AST_EMPTY_STMT:
        return ast_node(Ast_Empty_Stmt, node)->token;
    case AST_BAD_STMT:
        return ast_node(Ast_Bad_Stmt, node)->end;
    case AST_EXPR_STMT:
        return ast_end_token(ast_node(Ast_Expr_Stmt, node)->expr);
    case AST_ASSIGNMENT: {
        ast_node_var(assignment, Ast_Assignment, node);
        if (assignment->rhs.count) {
            return ast_end_token(assignment->rhs.back());
        }
        return assignment->token;
    }
    case AST_IF: {
        ast_node_var(if_stmt, Ast_If, node);
        return ast_end_token(if_stmt->block);
    }
    case AST_IFCASE: 
        return ast_node(Ast_Ifcase, node)->close;
    case AST_CASE_LABEL: {
        ast_node_var(label, Ast_Case_Label, node);
        if (label->statements.count) {
            return ast_end_token(label->statements.back());
        }
        return label->token;
    }
    case AST_DO_WHILE:
        return ast_node(Ast_Do_While, node)->token;
    case AST_WHILE: {
        return ast_node(Ast_While, node)->token;
    }
    case AST_FOR:
        return ast_node(Ast_For, node)->token;
    case AST_BLOCK:
        return ast_node(Ast_Continue, node)->token;
    case AST_RETURN: {
        ast_node_var(return_stmt, Ast_Return, node);
        if (return_stmt->values.count > 0) {
            return ast_end_token(return_stmt->values.back());
        } else {
            return return_stmt->token;
        }
    }
    case AST_CONTINUE:
        return ast_node(Ast_Continue, node)->token;
    case AST_BREAK:
        return ast_node(Ast_Break, node)->token;
    case AST_FALLTHROUGH:
        return ast_node(Ast_Fallthrough, node)->token;
    case AST_LOAD_STMT:
        return ast_node(Ast_Load, node)->token;
    case AST_IMPORT_STMT:
        return ast_node(Ast_Import, node)->token;
    case AST_DEFER_STMT:
        return ast_node(Ast_Defer, node)->token;

    case AST_BAD_EXPR:
        return ast_node(Ast_Bad_Expr, node)->start;
    case AST_PAREN:
        return ast_node(Ast_Paren, node)->close;
    case AST_LITERAL:
        return ast_node(Ast_Literal, node)->token;
    case AST_COMPOUND_LITERAL:
        return ast_node(Ast_Compound_Literal, node)->close;
    case AST_IDENT:
        return ast_node(Ast_Ident, node)->token;
    case AST_CALL:
        return ast_node(Ast_Call, node)->close;
    case AST_SUBSCRIPT:
        return ast_node(Ast_Subscript, node)->close;
    case AST_CAST:
        return ast_node(Ast_Cast, node)->token;
    case AST_ITERATOR:
        break;
    case AST_UNARY:
        return ast_node(Ast_Unary, node)->token;
    case AST_ADDRESS:
        return ast_node(Ast_Address, node)->token;
    case AST_DEREF:
        return ast_node(Ast_Deref, node)->token;
    case AST_BINARY: {
        ast_node_var(binary, Ast_Binary, node);
        return ast_start_token(binary->lhs);
    }
    case AST_SELECTOR: {
        ast_node_var(selector, Ast_Selector, node);
        return ast_end_token(selector->name);
    }
    case AST_RANGE: {
        ast_node_var(range, Ast_Range, node);
        return ast_start_token(range->lhs);
    }
    case AST_SIZEOF:
        return ast_node(Ast_Sizeof, node)->token;

    case AST_POINTER_TYPE:
        return ast_end_token(ast_node(Ast_Pointer_Type, node)->elem);
    case AST_ARRAY_TYPE:
        return ast_end_token(ast_node(Ast_Array_Type, node)->elem);
    case AST_PROC_TYPE:
        return ast_node(Ast_Proc_Type, node)->close;
    case AST_ENUM_TYPE:
        return ast_node(Ast_Enum_Type, node)->close;
    case AST_STRUCT_TYPE:
        return ast_node(Ast_Struct_Type, node)->close;
    case AST_UNION_TYPE:
        return ast_node(Ast_Union_Type, node)->close;
    }
    return {};
}
