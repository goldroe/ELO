global Arena *g_ast_arena;
global u64 g_ast_counter;

internal bool is_ast_type(Ast *node) {
    switch (node->kind) {
    case AST_STRUCT_TYPE:
    case AST_ENUM_TYPE:
    case AST_UNION_TYPE:
    case AST_POINTER_TYPE:
    case AST_ARRAY_TYPE:
        return true;
    }
    return false;
}

internal inline Allocator ast_allocator() {
    return arena_allocator(g_ast_arena);
}

internal inline Ast *ast__init(Ast *node, Source_File *f) {
    node->file = f;
    node->id = g_ast_counter++;
    return node;
}

internal inline Ast *ast_alloc(u64 size, int alignment) {
    Ast *node = (Ast *)arena_alloc(g_ast_arena, size, alignment);
    return node;
}

internal Ast_Root *ast_root(Source_File *f, Array<Ast*> decls) {
    Ast_Root *node = AST_NEW(f, Ast_Root);
    node->decls = decls;
    return node;
}

internal Ast_Empty_Stmt *ast_empty_stmt(Source_File *f, Token token) {
    Ast_Empty_Stmt *node = AST_NEW(f, Ast_Empty_Stmt);
    node->token = token;
    return node;
}

internal Ast_Bad_Expr *ast_bad_expr(Source_File *f, Token start, Token end) {
    Ast_Bad_Expr *node = AST_NEW(f, Ast_Bad_Expr);
    node->poison();
    node->start = start;
    node->end = end;
    return node;
}

internal Ast_Bad_Stmt *ast_bad_stmt(Source_File *f, Token start, Token end) {
    Ast_Bad_Stmt *node = AST_NEW(f, Ast_Bad_Stmt);
    node->poison();
    node->start = start;
    node->end = end;
    return node;
}

internal Ast_Bad_Decl *ast_bad_decl(Source_File *f, Token start, Token end) {
    Ast_Bad_Decl *node = AST_NEW(f, Ast_Bad_Decl);
    node->poison();
    node->start = start;
    node->end = end;
    return node;
}

internal Ast_Return *ast_return_stmt(Source_File *f, Token token, Array<Ast*> values) {
    Ast_Return *node = AST_NEW(f, Ast_Return);
    node->values = values;
    node->token = token;
    return node;
}

internal Ast_Continue *ast_continue_stmt(Source_File *f, Token token) {
    Ast_Continue *node = AST_NEW(f, Ast_Continue);
    node->token = token;
    return node;
}

internal Ast_Break *ast_break_stmt(Source_File *f, Token token) {
    Ast_Break *node = AST_NEW(f, Ast_Break);
    node->token = token;
    return node;
}

internal Ast_Fallthrough *ast_fallthrough_stmt(Source_File *f, Token token) {
    Ast_Fallthrough *node = AST_NEW(f, Ast_Fallthrough);
    node->token = token;
    return node;
}

internal Ast_Defer *ast_defer_stmt(Source_File *f, Token token, Ast *stmt) {
    Ast_Defer *node = AST_NEW(f, Ast_Defer);
    node->token = token;
    node->stmt = stmt;
    return node;
}

internal Ast_Proc_Lit *ast_proc_lit(Source_File *f, Ast_Proc_Type *typespec, Ast_Block *body) {
    Ast_Proc_Lit *node = AST_NEW(f, Ast_Proc_Lit);
    node->typespec = typespec;
    node->body = body;
    array_init(&node->local_vars, heap_allocator());
    return node;
}

internal Ast_Enum_Field *ast_enum_field(Source_File *f, Ast_Ident *ident, Ast *expr) {
    Ast_Enum_Field *node = AST_NEW(f, Ast_Enum_Field);
    node->mode = ADDRESSING_CONSTANT;
    node->name = ident;
    node->expr = expr;
    return node;
}

internal Ast_Param *ast_param(Source_File *f, Ast_Ident *name, Ast *typespec) {
    Ast_Param *node = AST_NEW(f, Ast_Param);
    node->name = name;
    node->typespec = typespec;
    return node;
}

internal Ast_Ident *ast_ident(Source_File *f, Token token) {
    Ast_Ident *node = AST_NEW(f, Ast_Ident);
    node->name = token.name;
    node->token = token;
    return node;
}

internal Ast_Literal *ast_literal(Source_File *f, Token token) {
    Ast_Literal *node = AST_NEW(f, Ast_Literal);
    node->value = token.value;
    node->token = token;
    return node;
}

internal Ast_Compound_Literal *ast_compound_literal(Source_File *f, Token open, Token close, Ast *typespec, Array<Ast*> elements) {
    Ast_Compound_Literal *node = AST_NEW(f, Ast_Compound_Literal);
    node->elements = elements;
    node->typespec = typespec;
    node->open = open;
    node->close = close;
    return node;
}

internal Ast_Paren *ast_paren_expr(Source_File *f, Token open, Token close, Ast *elem) {
    Ast_Paren *node = AST_NEW(f, Ast_Paren);
    node->elem = elem;
    node->flags = elem->flags;
    node->open = open;
    node->close = close;
    return node;
}

internal Ast_Unary *ast_unary_expr(Source_File *f, Token token, OP op, Ast *elem) {
    Ast_Unary *node = AST_NEW(f, Ast_Unary);
    node->op = op;
    node->elem = elem;
    node->token = token;
    return node;
}

internal Ast_Address *ast_address_expr(Source_File *f, Token token, Ast *elem) {
    Ast_Address *node = AST_NEW(f, Ast_Address);
    node->elem = elem;
    node->flags |= AST_FLAG_LVALUE;
    node->token = token;
    return node;
}

internal Ast_Range *ast_range_expr(Source_File *f, Token token, Ast *lhs, Ast *rhs) {
    Ast_Range *node = AST_NEW(f, Ast_Range);
    node->lhs = lhs;
    node->rhs = rhs;
    node->token = token;
    return node;
}

internal Ast_Deref *ast_deref_expr(Source_File *f, Token token, Ast *elem) {
    Ast_Deref *node = AST_NEW(f, Ast_Deref);
    node->elem = elem;
    node->flags |= AST_FLAG_LVALUE;
    node->token = token;
    return node;
}

internal Ast_Cast *ast_cast_expr(Source_File *f, Token token, Ast *typespec, Ast *elem) {
    Ast_Cast *node = AST_NEW(f, Ast_Cast);
    node->typespec = typespec;
    node->elem = elem;
    node->flags = elem->flags;
    node->token = token;
    return node;
}

internal Ast_Call *ast_call_expr(Source_File *f, Token open, Token close, Ast *elem, Array<Ast*> arguments) {
    Ast_Call *node = AST_NEW(f, Ast_Call);
    node->elem = elem;
    node->arguments = arguments;
    node->open = open;
    node->close = close;
    return node;
}

internal Ast_Selector *ast_selector_expr(Source_File *f, Token token, Ast *parent, Ast_Ident *name) {
    Ast_Selector *node = AST_NEW(f, Ast_Selector);
    node->parent = parent;
    node->name = name;
    node->token = token;
    return node;
}

internal Ast_Value_Decl *ast_value_decl(Source_File *f, Array<Ast*> names, Ast *typespec, Array<Ast*> values, bool is_mutable) {
    Ast_Value_Decl *node = AST_NEW(f, Ast_Value_Decl);
    node->names = names;
    node->typespec = typespec;
    node->values = values;
    node->is_mutable = is_mutable;
    return node;
}

internal Ast_Load *ast_load_stmt(Source_File *f, Token token, Token file_token, String8 file_path) {
    Ast_Load *node = AST_NEW(f, Ast_Load);
    node->rel_path = file_path;
    node->token = token;
    node->file_token = file_token;
    return node;
}

internal Ast_Import *ast_import_stmt(Source_File *f, Token token, Token file_token, String8 file_path) {
    Ast_Import *node = AST_NEW(f, Ast_Import);
    node->rel_path = file_path;
    node->token = token;
    node->file_token = file_token;
    return node;
}

internal Ast_Binary *ast_binary_expr(Source_File *f, Token token, OP op, Ast *lhs, Ast *rhs) {
    Ast_Binary *node = AST_NEW(f, Ast_Binary);
    node->op = op;
    node->lhs = lhs;
    node->rhs = rhs;
    node->token = token;
    return node; 
}

internal Ast_Assignment *ast_assignment_stmt(Source_File *f, Token token, OP op, Array<Ast*> lhs, Array<Ast*> rhs) {
    Ast_Assignment *node = AST_NEW(f, Ast_Assignment);
    node->op = op;
    node->lhs = lhs;
    node->rhs = rhs;
    node->token = token;
    return node;
}

internal Ast_Subscript *ast_subscript_expr(Source_File *f, Token open, Token close, Ast *base, Ast *index) {
    Ast_Subscript *node = AST_NEW(f, Ast_Subscript);
    node->expr = base;
    node->index = index;
    node->flags |= AST_FLAG_LVALUE;
    node->open = open;
    node->close = close;
    return node;
}

internal Ast_Sizeof *ast_sizeof_expr(Source_File *f, Token token, Token open, Token close, Ast *elem) {
    Ast_Sizeof *node = AST_NEW(f, Ast_Sizeof);
    node->elem = elem;
    node->token = token;
    node->open = open;
    node->close = close;
    return node;
}

internal Ast_Expr_Stmt *ast_expr_stmt(Source_File *f, Ast *expr) {
    Ast_Expr_Stmt *node = AST_NEW(f, Ast_Expr_Stmt);
    node->expr = expr;
    return node;
}

internal Ast_Block *ast_block_stmt(Source_File *f, Token open, Token close, Array<Ast*> statements) {
    Ast_Block *node = AST_NEW(f, Ast_Block);
    node->statements = statements;
    node->open = open;
    node->close = close;
    return node;
}

internal Ast_If *ast_if_stmt(Source_File *f, Token token, Ast *cond, Ast_Block *block) {
    Ast_If *node = AST_NEW(f, Ast_If);
    node->stmt_flags = STMT_FLAG_PATH_BRANCH;
    node->cond = cond;
    node->block = block;
    node->token = token;
    return node;
}

internal Ast_Case_Label *ast_case_label(Source_File *f, Token token, Ast *cond, Array<Ast*> statements) {
    Ast_Case_Label *node = AST_NEW(f, Ast_Case_Label);
    node->cond = cond;
    node->statements = statements;
    node->token = token;
    return node;
}

internal Ast_Ifcase *ast_ifcase_stmt(Source_File *f, Token token, Token open, Token close, Ast *cond, Array<Ast_Case_Label*> clauses, bool check_enum_complete) {
    Ast_Ifcase *node = AST_NEW(f, Ast_Ifcase);
    node->cond = cond;
    node->cases = clauses;
    node->check_enum_complete = check_enum_complete;
    node->token = token;
    node->open = open;
    node->close = close;
    return node;
}

internal Ast_While *ast_while_stmt(Source_File *f, Token token, Ast *cond, Ast_Block *block) {
    Ast_While *node = AST_NEW(f, Ast_While);
    node->cond = cond;
    node->block = block;
    node->token = token;
    return node;
}

internal Ast_For *ast_for_stmt(Source_File *f, Token token, Array<Ast*> lhs, Ast *range_expr, Ast_Block *block) {
    Ast_For *node = AST_NEW(f, Ast_For);
    node->lhs = lhs;
    node->range_expr = range_expr;
    node->block = block;
    node->token = token;
    return node;
} 

internal Ast_Pointer_Type *ast_pointer_type(Source_File *f, Token token, Ast *elem) {
    Ast_Pointer_Type *node = AST_NEW(f, Ast_Pointer_Type);
    node->elem = elem;
    node->token = token;
    return node;
}

internal Ast_Array_Type *ast_array_type(Source_File *f, Token token, Ast *elem, Ast *length) {
    Ast_Array_Type *node = AST_NEW(f, Ast_Array_Type);
    node->elem = elem;
    node->length = length;
    node->token = token;
    return node;
}

internal Ast_Proc_Type *ast_proc_type(Source_File *f, Token open, Token close, Array<Ast_Param*> params, Array<Ast*> results) {
    Ast_Proc_Type *node = AST_NEW(f, Ast_Proc_Type);
    node->params = params;
    node->results = results;
    node->open = open;
    node->close = close;
    return node;
}

internal Ast_Enum_Type *ast_enum_type(Source_File *f, Token token, Token open, Token close, Ast *base_type, Array<Ast_Enum_Field*> fields) {
    Ast_Enum_Type *node = AST_NEW(f, Ast_Enum_Type);
    node->base_type = base_type;
    node->fields = fields;
    node->token = token;
    node->open = open;
    node->close = close;
    return node;
}

internal Ast_Struct_Type *ast_struct_type(Source_File *f, Token token, Token open, Token close, Array<Ast_Value_Decl*> members) {
    Ast_Struct_Type *node = AST_NEW(f, Ast_Struct_Type);
    node->members = members;
    node->token = token;
    node->open = open;
    node->close = close;
    return node;
}

internal Ast_Union_Type *ast_union_type(Source_File *f, Token token, Token open, Token close, Array<Ast_Value_Decl*> members) {
    Ast_Union_Type *node = AST_NEW(f, Ast_Union_Type);
    node->members = members;
    node->token = token;
    node->open = open;
    node->close = close;
    return node;
}

internal char *string_from_expr(Ast *expr) {
    if (expr == NULL) return NULL;
    
    cstring result = NULL;
    switch (expr->kind) {
    case AST_CAST: {
        Ast_Cast *cast = static_cast<Ast_Cast*>(expr);
        cstring str = make_cstring("cast(");
        str = cstring_append(str, string_from_type(cast->type));
        str = cstring_append(str, ")");
        str = cstring_append(str, string_from_expr(cast->elem));
        result = str;
        break;
    }
    case AST_PAREN: {
        Ast_Paren *paren = (Ast_Paren *)expr;
        cstring str = make_cstring("(");
        str = cstring_append(str, string_from_expr(paren->elem));
        str = cstring_append(str, ")");
        result = str;
        break;
    }
    case AST_LITERAL: {
        Ast_Literal *literal = (Ast_Literal *)expr;
        switch (literal->value.kind) {
        case CONSTANT_VALUE_INTEGER: {
            String string = string_from_bigint(literal->value.value_integer);
            result = cstring_fmt("%S", string);
            break;
        }
        case CONSTANT_VALUE_FLOAT:
            result = cstring_fmt("%f", literal->value.value_float);
            break;
        case CONSTANT_VALUE_STRING: {
            String string = literal->value.value_string;
            result = make_cstring_len((const char *)string.data, string.count);
            break;
        }
        }
        break;
    }
    case AST_IDENT: {
        Ast_Ident *ident = (Ast_Ident *)expr;
        result = make_cstring_len((const char *)ident->name->data, ident->name->count);
        break;
    }
    case AST_CALL: {
        Ast_Call *call = (Ast_Call *)expr;
        cstring str = string_from_expr(call->elem);
        str = cstring_append(str, "(..)");
        result = str;
        break;
    }
    case AST_SUBSCRIPT: {
        Ast_Subscript *subscript = (Ast_Subscript *)expr;
        cstring str = string_from_expr(subscript->expr);
        str = cstring_append(str, "[");
        str = cstring_append(str, string_from_expr(subscript->index));
        str = cstring_append(str, "]");
        result = str;
        break;
    }
    case AST_UNARY: {
        Ast_Unary *unary = (Ast_Unary *)expr;
        cstring str = string_from_operator(unary->op);
        str = cstring_append(str, string_from_expr(unary->elem));
        result = str;
        break;
    }
    case AST_ADDRESS: {
        Ast_Address *address = (Ast_Address *)expr;
        cstring str = make_cstring("*");
        str = cstring_append(str, string_from_expr(address->elem));
        result = str;
        break;
    }
    case AST_DEREF: {
        Ast_Deref *deref = (Ast_Deref *)expr;
        cstring str = string_from_expr(deref->elem);
        str = cstring_append(str, ".*");
        result = str;
        break;
    }
    case AST_BINARY: {
        Ast_Binary *binary = (Ast_Binary *)expr;
        cstring str = string_from_expr(binary->lhs);
        str = cstring_append(str, string_from_operator(binary->op));
        str = cstring_append(str, string_from_expr(binary->rhs));
        result = str;
        break;
    }
    case AST_RANGE: {
        Ast_Range *range = static_cast<Ast_Range*>(expr);
        cstring str = cstring_fmt("%s..%s", string_from_expr(range->lhs), string_from_expr(range->rhs));
        result = str;
        break;
    }
    case AST_SELECTOR: {
        Ast_Selector *selector = (Ast_Selector *)expr;
        cstring str = NULL;
        str = string_from_expr(selector->parent);
        str = cstring_append(str, ".");
        str = cstring_append(str, string_from_expr(selector->name));
        result = str;
        break;
    }
    case AST_COMPOUND_LITERAL: {
        break;
    }
    }

    return result;
}

internal inline const char *string_from_ast(Ast_Kind kind) {
    return ast_strings[kind];
}

internal char *string_from_operator(OP op) {
    switch (op) {
    default:
        Assert(0);
        return "";
    case OP_ADD:
        return "+";
    case OP_SUB:
        return "-";
    case OP_MUL:
        return "*";
    case OP_DIV:
        return "/";
    case OP_MOD:
        return "%";
    case OP_UNARY_MINUS:
        return "-";
    case OP_UNARY_PLUS:
        return "+";
    case OP_EQ:
        return "==";
    case OP_NEQ:
        return "!=";
    case OP_LT:
        return "<";
    case OP_LTEQ:
        return "<=";
    case OP_GT:
        return ">";
    case OP_GTEQ:
        return ">=";
    case OP_NOT:
        return "!";
    case OP_OR:
        return "||";
    case OP_AND:
        return "&&";
    case OP_BIT_NOT:
        return "~";
    case OP_BIT_AND:
        return "&";
    case OP_BIT_OR:
        return "|";
    case OP_XOR:
        return "^";
    case OP_LSH:
        return "<<";
    case OP_RSH:
        return ">>";
    case OP_ASSIGN:
        return "=";
    case OP_ADD_ASSIGN:
        return "+=";
    case OP_SUB_ASSIGN:
        return "-=";
    case OP_MUL_ASSIGN:
        return "*=";
    case OP_DIV_ASSIGN:
        return "/=";
    case OP_MOD_ASSIGN:
        return "%/=";
    case OP_AND_ASSIGN:
        return "&=";
    case OP_OR_ASSIGN:
        return "|=";
    case OP_XOR_ASSIGN:
        return "^=";
    case OP_LSH_ASSIGN:
        return "<<=";
    case OP_RSH_ASSIGN:
        return ">>=";
    case OP_ADDRESS:
        return "*";
    case OP_DEREF:
        return ".*";
    case OP_SUBSCRIPT:
        return "[]";
    case OP_SELECT:
        return ".";
    case OP_CAST:
        return "cast";
    }
}

internal inline OP get_unary_operator(Token_Kind kind) {
    switch (kind) {
    default:
        return OP_ERR;
    case TOKEN_CAST:
        return OP_CAST;
    case TOKEN_PLUS:
        return OP_UNARY_PLUS;
    case TOKEN_MINUS:
        return OP_UNARY_MINUS;
    case TOKEN_BANG:
        return OP_NOT;
    case TOKEN_SQUIGGLE:
        return OP_BIT_NOT;
    }
}

internal inline OP get_binary_operator(Token_Kind kind) {
    switch (kind) {
    default:
        return OP_ERR;

    case TOKEN_PLUS:
        return OP_ADD;
    case TOKEN_MINUS:
        return OP_SUB;
    case TOKEN_STAR:
        return OP_MUL;
    case TOKEN_SLASH:
        return OP_DIV;
    case TOKEN_MOD:
        return OP_MOD;

    case TOKEN_EQ2:
        return OP_EQ;
    case TOKEN_NEQ:
        return OP_NEQ;
    case TOKEN_LT:
        return OP_LT;
    case TOKEN_GT:
        return OP_GT;
    case TOKEN_LTEQ:
        return OP_LTEQ;
    case TOKEN_GTEQ:
        return OP_GTEQ;

    case TOKEN_BAR:
        return OP_BIT_OR;
    case TOKEN_AMPER:
        return OP_BIT_AND;
    case TOKEN_OR:
        return OP_OR;
    case TOKEN_AND:
        return OP_AND;
    case TOKEN_XOR:
        return OP_XOR;

    case TOKEN_LSHIFT:
        return OP_LSH;
    case TOKEN_RSHIFT:
        return OP_RSH;

    case TOKEN_EQ:
        return OP_ASSIGN;
    case TOKEN_PLUS_EQ:
        return OP_ADD_ASSIGN;
    case TOKEN_MINUS_EQ:
        return OP_SUB_ASSIGN;
    case TOKEN_STAR_EQ:
        return OP_MUL_ASSIGN;
    case TOKEN_SLASH_EQ:
        return OP_DIV_ASSIGN;
    case TOKEN_MOD_EQ:
        return OP_MOD_ASSIGN;
    case TOKEN_AMPER_EQ:
        return OP_AND_ASSIGN;
    case TOKEN_BAR_EQ:
        return OP_OR_ASSIGN;
    case TOKEN_XOR_EQ:
        return OP_XOR_ASSIGN;
    case TOKEN_LSHIFT_EQ:
        return OP_LSH_ASSIGN;
    case TOKEN_RSHIFT_EQ:
        return OP_RSH_ASSIGN;
    }
}

internal inline int get_operator_precedence(OP op) {
    switch (op) {
    default:
    case OP_ERR:
        return -1;

    case OP_SUBSCRIPT:
    case OP_SELECT:
        return 13000;

    case OP_UNARY_PLUS:
    case OP_UNARY_MINUS:
    case OP_NOT:
    case OP_BIT_NOT:
    case OP_DEREF:
    case OP_ADDRESS:
    case OP_CAST:
        return 12000;

    case OP_MUL:
    case OP_DIV:
    case OP_MOD:
        return 11000;

    case OP_ADD:
    case OP_SUB:
        return 10000;

    case OP_LT:
    case OP_LTEQ:
    case OP_GT:
    case OP_GTEQ:
        return 9000;

    case OP_EQ:
    case OP_NEQ:
        return 8000;

    case OP_BIT_AND:
        return 70000;

    case OP_XOR:
        return 6000;

    case OP_BIT_OR:
        return 5000;

    case OP_AND:
        return 4000;

    case OP_OR:
        return 3000;

    case OP_LSH:
    case OP_RSH:
        return 2000;

    case OP_ASSIGN:
    case OP_ADD_ASSIGN:
    case OP_SUB_ASSIGN:
    case OP_MUL_ASSIGN:
    case OP_DIV_ASSIGN:
    case OP_MOD_ASSIGN:
    case OP_LSHIFT_ASSIGN:
    case OP_RSHIFT_ASSIGN:
    case OP_AND_ASSIGN:
    case OP_OR_ASSIGN:
    case OP_XOR_ASSIGN:
        return -1;
        // return 1000;
    }
}

internal inline bool is_assignment_op(OP op) {
    return OP_ASSIGN <= op && op <= OP_ASSIGN_END;
}

inline bool is_binop(Ast *expr, OP op) {
    return expr->kind == AST_BINARY && ((Ast_Binary *)expr)->op == op;
}

