global Arena *g_ast_arena;

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
    case OP_ACCESS:
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

inline bool Ast_Expr::is_binop(OP binop) {
    return kind == AST_BINARY && static_cast<Ast_Binary*>(this)->op == binop;
}

internal Ast *ast_alloc(u64 size, int alignment) {
    Ast *node = (Ast *)arena_alloc(g_ast_arena, size, alignment);
    return node;
}

internal inline Allocator ast_allocator() {
    return arena_allocator(g_ast_arena);
}

Ast_Enum_Field *lookup_field(Ast_Enum *enum_decl, Atom *name) {
    Ast_Enum_Field *result = NULL;
    for (int i = 0; i < enum_decl->fields.count; i++) {
        Ast_Enum_Field *field = enum_decl->fields[i];
        if (atoms_match(name, field->name)) {
            return field;
        }
    }
    return NULL;
}

Ast_Decl *Scope::lookup(Atom *name) {
    for (Scope *scope = this; scope; scope = scope->parent) {
        for (int i = 0; i < scope->declarations.count; i++) {
            Ast_Decl *decl = scope->declarations[i];
            if (atoms_match(decl->name, name)) {
                return decl;
            }
        }
    }
    return NULL;
}

internal Scope *make_scope(Scope_Flags flags) {
    Scope *result = (Scope*)ast_alloc(sizeof(Scope), alignof(Scope));
    result->scope_flags = flags;
    return result;
}

internal Ast_Continue *ast_continue_stmt(Token t) {
    Ast_Continue *result = AST_NEW(Ast_Continue);
    result->mark_range(t.start, t.end);
    return result;
}

internal Ast_Break *ast_break_stmt(Token t) {
    Ast_Break *result = AST_NEW(Ast_Break);
    result->mark_range(t.start, t.end);
    return result;
}

internal Ast_Fallthrough *ast_fallthrough_stmt(Token t) {
    Ast_Fallthrough *result = AST_NEW(Ast_Fallthrough);
    result->mark_range(t.start, t.end);
    return result;
}

internal Ast_Return *ast_return_stmt(Ast_Expr *expr) {
    Ast_Return *result = AST_NEW(Ast_Return);
    result->expr = expr;
    return result;
}

internal Ast_Empty_Stmt *ast_empty_stmt(Token t) {
    Ast_Empty_Stmt *result = AST_NEW(Ast_Empty_Stmt);
    result->mark_range(t.start, t.end);
    return result;
}

internal Ast_Type_Decl *ast_type_decl(Atom *name, Type *type) {
    Ast_Type_Decl *result = AST_NEW(Ast_Type_Decl);
    result->decl_flags |= DECL_FLAG_TYPE;
    result->name = name;
    result->type = type;
    return result;
}

internal Ast_Var *ast_var(Atom *name, Ast_Expr *init, Ast_Type_Defn *type_defn) {
    Ast_Var *result = AST_NEW(Ast_Var);
    result->name = name;
    result->init = init;
    result->type_defn = type_defn;
    return result;
}

internal Ast_Proc *ast_proc(Atom *name, Auto_Array<Ast_Param*> parameters, Ast_Type_Defn *return_type, Ast_Block *block) {
    Ast_Proc *result = AST_NEW(Ast_Proc);
    result->name = name;
    result->parameters = parameters;
    result->return_type_defn = return_type;
    result->block = block;
    return result;
}

internal Ast_Operator_Proc *ast_operator_proc(OP op, Auto_Array<Ast_Param*> parameters, Ast_Type_Defn *return_type, Ast_Block *block) {
    Ast_Operator_Proc *result = AST_NEW(Ast_Operator_Proc);
    result->name = atom_create(str8_pushf(ast_allocator(), "operator%s", string_from_operator(op)));
    result->op = op;
    result->parameters = parameters;
    result->return_type_defn = return_type;
    result->block = block;
    return result;
}

internal Ast_Struct *ast_struct(Atom *name, Auto_Array<Ast_Decl*> members) {
    Ast_Struct *result = AST_NEW(Ast_Struct);
    result->decl_flags |= DECL_FLAG_TYPE;
    result->name = name;
    result->members = members;
    return result;
}

// internal Ast_Struct_Field *ast_struct_field(Atom *name, Ast_Type_Defn *type_defn) {
//     Ast_Struct_Field *result = AST_NEW(Ast_Struct_Field);
//     result->name = name;
//     result->type_defn = type_defn;
//     return result;
// }

internal Ast_Enum *ast_enum(Atom *name, Auto_Array<Ast_Enum_Field*> fields) {
    Ast_Enum *result = AST_NEW(Ast_Enum);
    result->decl_flags |= DECL_FLAG_TYPE;
    result->name = name;
    result->fields = fields;
    return result;
}

internal Ast_Enum_Field *ast_enum_field(Atom *name) {
    Ast_Enum_Field *result = AST_NEW(Ast_Enum_Field);
    result->decl_flags |= DECL_FLAG_CONST;
    result->name = name;
    return result;
}

internal Ast_Param *ast_param(Atom *name, Ast_Type_Defn *type_defn) {
    Ast_Param *result = AST_NEW(Ast_Param);
    result->name = name;
    result->type_defn = type_defn;
    return result;
}

internal Ast_Type_Defn *ast_type_defn(Type_Defn_Kind kind, Ast_Type_Defn *base) {
    Ast_Type_Defn *result = AST_NEW(Ast_Type_Defn);
    result->type_defn_kind = kind;
    result->base = base;
    return result;
}

internal Ast_Paren *ast_paren(Ast_Expr *elem) {
    Ast_Paren *result = AST_NEW(Ast_Paren);
    result->elem = elem;
    result->expr_flags = elem->expr_flags;
    return result;
}

internal Ast_Ident *ast_ident(Token name) {
    Ast_Ident *result = AST_NEW(Ast_Ident);
    result->mark_range(name.start, name.end);
    result->name = name.name;
    return result;
}

internal Ast_Literal *ast_intlit(Token token) {
    Ast_Literal *result = AST_NEW(Ast_Literal);
    result->expr_flags |= EXPR_FLAG_CONSTANT;
    result->literal_flags = token.literal_flags;
    result->int_val = token.intlit;
    return result;
}

internal Ast_Literal *ast_floatlit(Token token) {
    Ast_Literal *result = AST_NEW(Ast_Literal);
    result->expr_flags |= EXPR_FLAG_CONSTANT;
    result->literal_flags = token.literal_flags;
    result->float_val = token.floatlit;
    return result;
}

internal Ast_Literal *ast_strlit(Token token) {
    Ast_Literal *result = AST_NEW(Ast_Literal);
    result->expr_flags |= EXPR_FLAG_CONSTANT;
    result->literal_flags = LITERAL_STRING;
    result->str_val = token.strlit;
    return result;
}

internal Ast_Compound_Literal *ast_compound_literal(Auto_Array<Ast_Expr*> elements, Ast_Type_Defn *type_defn) {
    Ast_Compound_Literal *result = AST_NEW(Ast_Compound_Literal);
    result->elements = elements;
    result->type_defn = type_defn;
    return result;
}

internal Ast_Unary *ast_unary_expr(OP op, Ast_Expr *elem) {
    Ast_Unary *result = AST_NEW(Ast_Unary);
    result->op = op;
    result->elem = elem;
    return result;
}

internal Ast_Address *ast_address_expr(Ast_Expr *elem) {
    Ast_Address *result = AST_NEW(Ast_Address);
    result->elem = elem;
    result->expr_flags |= EXPR_FLAG_LVALUE;
    // result->op = OP_ADDRESS;
    return result;
}

internal Ast_Range *ast_range_expr(Ast_Expr *lhs, Ast_Expr *rhs) {
    Ast_Range *result = AST_NEW(Ast_Range);
    result->lhs = lhs;
    result->rhs = rhs;
    return result;
}

internal Ast_Deref *ast_deref_expr(Token op, Ast_Expr *elem) {
    Ast_Deref *result = AST_NEW(Ast_Deref);
    result->mark_start(op.start);
    result->mark_start(elem->end);
    result->elem = elem;
    result->expr_flags |= EXPR_FLAG_LVALUE;
    return result;
}

internal Ast_Cast *ast_cast_expr(Ast_Type_Defn *type, Ast_Expr *elem) {
    Ast_Cast *result = AST_NEW(Ast_Cast);
    result->type_defn = type;
    result->elem = elem;
    result->expr_flags = elem->expr_flags;
    return result;
}

internal Ast_Call *ast_call_expr(Token op, Ast_Expr *elem, Auto_Array<Ast_Expr*> arguments) {
    Ast_Call *result = AST_NEW(Ast_Call);
    result->elem = elem;
    result->arguments = arguments;
    return result;
}

internal Ast_Access *ast_access_expr(Ast_Expr *parent, Ast_Ident *name) {
    Ast_Access *result = AST_NEW(Ast_Access);
    result->parent = parent;
    result->name = name;
    return result;
}

internal Ast_Expr *ast_error_expr() {
    Ast_Expr *result = AST_NEW(Ast_Expr);
    result->poison();
    return result;
}

internal Ast_Bad_Stmt *ast_bad_stmt(Token start, Token end) {
    Ast_Bad_Stmt *result = AST_NEW(Ast_Bad_Stmt);
    result->poison();
    result->mark_range(start.start, end.end);
    return result;
}

internal Ast_Bad_Decl *ast_bad_decl(Token start, Token end) {
    Ast_Bad_Decl *result = AST_NEW(Ast_Bad_Decl);
    result->poison();
    result->mark_range(start.start, end.end);
    return result;
}

internal Ast_Load *ast_load_stmt(String8 file_path) {
    Ast_Load *result = AST_NEW(Ast_Load);
    result->rel_path = file_path;
    return result;
}

internal Ast_Import *ast_import_stmt(String8 file_path) {
    Ast_Import *result = AST_NEW(Ast_Import);
    result->rel_path = file_path;
    return result;
}

internal Ast_Binary *ast_binary_expr(OP op, Ast_Expr *lhs, Ast_Expr *rhs) {
    Ast_Binary *result = AST_NEW(Ast_Binary);
    result->op = op;
    result->lhs = lhs;
    result->rhs = rhs;
    if (lhs) result->mark_start(lhs->start);
    if (rhs) result->mark_end(rhs->end);
    return result; 
}

internal Ast_Assignment *ast_assignment_expr(OP op, Ast_Expr *lhs, Ast_Expr *rhs) {
    Ast_Assignment *result = AST_NEW(Ast_Assignment);
    result->expr_flags |= EXPR_FLAG_LVALUE;
    result->op = op;
    result->lhs = lhs;
    result->rhs = rhs;
    result->mark_range(lhs->start, rhs->end);
    return result;
}

internal Ast_Subscript *ast_subscript_expr(Token op, Ast_Expr *base, Ast_Expr *index) {
    Ast_Subscript *result = AST_NEW(Ast_Subscript);
    // result->loc = op.l0;
    result->expr = base;
    result->index = index;
    result->expr_flags |= EXPR_FLAG_LVALUE;
    return result;
}

internal Ast_Range *ast_range_expr(Token op, Ast_Expr *lhs, Ast_Expr *rhs) {
    Ast_Range *result = AST_NEW(Ast_Range);
    // result->loc = op.l0;
    result->lhs = lhs;
    result->rhs = rhs;
    return result;
}

internal Ast_Decl_Stmt *ast_decl_stmt(Ast_Decl *decl) {
    Ast_Decl_Stmt *result = AST_NEW(Ast_Decl_Stmt);
    result->decl = decl;
    return result;
}

internal Ast_Expr_Stmt *ast_expr_stmt(Ast_Expr *expr) {
    Ast_Expr_Stmt *result = AST_NEW(Ast_Expr_Stmt);
    result->expr = expr;
    return result;
}

internal Ast_If *ast_if_stmt(Ast_Expr *cond, Ast_Block *block) {
    Ast_If *result = AST_NEW(Ast_If);
    result->stmt_flags = STMT_FLAG_PATH_BRANCH;
    result->cond = cond;
    result->block = block;
    return result;
}

internal Ast_While *ast_while_stmt(Ast_Expr *cond, Ast_Block *block) {
    Ast_While *result = AST_NEW(Ast_While);
    result->cond = cond;
    result->block = block;
    return result;
}

internal Ast_For *ast_for_stmt(Atom *name, Ast_Expr *iterator, Ast_Block *block) {
    Ast_For *result = AST_NEW(Ast_For);
    Ast_Var *var = ast_var(name, iterator, NULL);
    result->var = var;
    result->iterator = iterator;
    result->block = block;
    return result;
} 

internal char *string_from_type(Type *ty) {
    if (ty == NULL) return "";
    cstring string = NULL;
    for (Type *type = ty; type; type = type->base) {
        switch (type->id) {
        case TYPEID_POINTER:
            string = cstring_append(string, "*");
            break;
        case TYPEID_ARRAY:
            string = cstring_append(string, "[..]");
            break;
        case TYPEID_PROC:
        {
            Proc_Type *proc_type = static_cast<Proc_Type*>(type);
            string = cstring_append(string, "(");
            for (Type *param : proc_type->parameters) {
                cstring_append(string, string_from_type(param));
                if (param == proc_type->parameters.back()) string = cstring_append(string, ",");
            }
            string = cstring_append(string, ")");
            if (proc_type->return_type) {
                string = cstring_append(string, "->(");
                string = cstring_append(string, string_from_type(proc_type->return_type));
                string = cstring_append(string, ")");
            }
            break;
        }
        default:
            string = cstring_append(string, type->decl->name->data);
            break;
        }
    }
    return string;
}

internal char *string_from_expr(Ast_Expr *expr) {
    if (expr == NULL) return NULL;
    
    cstring result = NULL;
    switch (expr->kind) {
    case AST_CAST:
    {
        Ast_Cast *cast = static_cast<Ast_Cast*>(expr);
        cstring str = make_cstring("cast(");
        str = cstring_append(str, string_from_type(cast->type));
        str = cstring_append(str, ")");
        str = cstring_append(str, string_from_expr(cast->elem));
        result = str;
        break;
    }
    case AST_PAREN:
    {
        Ast_Paren *paren = (Ast_Paren *)expr;
        cstring str = make_cstring("(");
        str = cstring_append(str, string_from_expr(paren->elem));
        str = cstring_append(str, ")");
        result = str;
        break;
    }
    case AST_LITERAL:
    {
        Ast_Literal *literal = (Ast_Literal *)expr;
        if (literal->literal_flags & LITERAL_INT) {
            result = cstring_fmt("%llu", literal->int_val);
        } else if (literal->literal_flags & LITERAL_FLOAT) {
            result = cstring_fmt("%f", literal->float_val);
        } else if (literal->literal_flags & LITERAL_STRING) {
            result = make_cstring_len((const char *)literal->str_val.data, literal->str_val.count); 
        }
        break;
    }
    case AST_IDENT:
    {
        Ast_Ident *ident = (Ast_Ident *)expr;
        result = make_cstring_len((const char *)ident->name->data, ident->name->count);
        break;
    }
    case AST_CALL:
    {
        Ast_Call *call = (Ast_Call *)expr;
        cstring str = string_from_expr(call->elem);
        str = cstring_append(str, "()");
        result = str;
        break;
    }
    case AST_SUBSCRIPT:
    {
        Ast_Subscript *subscript = (Ast_Subscript *)expr;
        cstring str = string_from_expr(subscript->expr);
        str = cstring_append(str, "[");
        str = cstring_append(str, string_from_expr(subscript->index));
        str = cstring_append(str, "]");
        result = str;
        break;
    }
    case AST_UNARY:
    {
        Ast_Unary *unary = (Ast_Unary *)expr;
        cstring str = string_from_operator(unary->op);
        str = cstring_append(str, string_from_expr(unary->elem));
        result = str;
        break;
    }
    case AST_ADDRESS:
    {
        Ast_Address *address = (Ast_Address *)expr;
        cstring str = make_cstring("*");
        str = cstring_append(str, string_from_expr(address->elem));
        result = str;
        break;
    }
    case AST_DEREF:
    {
        Ast_Deref *deref = (Ast_Deref *)expr;
        cstring str = string_from_expr(deref->elem);
        str = cstring_append(str, ".*");
        result = str;
        break;
    }
    case AST_BINARY:
    {
        Ast_Binary *binary = (Ast_Binary *)expr;
        cstring str = string_from_expr(binary->lhs);
        str = cstring_append(str, string_from_operator(binary->op));
        str = cstring_append(str, string_from_expr(binary->rhs));
        result = str;
        break;
    }
    case AST_RANGE:
    {
        Ast_Range *range = static_cast<Ast_Range*>(expr);
        cstring str = cstring_fmt("%s..%s", string_from_expr(range->lhs), string_from_expr(range->rhs));
        result = str;
        break;
    }
    case AST_ACCESS:
    {
        Ast_Access *access = (Ast_Access *)expr;
        cstring str = NULL;
        str = string_from_expr(access->parent);
        str = cstring_append(str, ".");
        str = cstring_append(str, string_from_expr(access->name));
        result = str;
        break;
    }
    case AST_COMPOUND_LITERAL:
    {
        break;
    }
    }

    return result;
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
    case OP_ACCESS:
        return ".";
    case OP_CAST:
        return "cast";
    }
}
