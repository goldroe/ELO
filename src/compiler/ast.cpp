global Arena *g_ast_arena;

Ast_Decl *Ast_Scope::lookup(Atom *name) {
    for (Ast_Scope *scope = this; scope; scope = scope->scope_parent) {
        for (int i = 0; i < scope->declarations.count; i++) {
            Ast_Decl *decl = scope->declarations[i];
            if (atoms_match(decl->name, name)) {
                return decl;
            }
        }
    }
    return NULL;
}

Auto_Array<Ast_Decl*> Ast_Scope::lookup_proc(Atom *name) {
    Auto_Array<Ast_Decl*> found;
    for (Ast_Scope *scope = this; scope; scope = scope->scope_parent) {
        for (int i = 0; i < scope->declarations.count; i++) {
            Ast_Decl *decl = scope->declarations[i];
            if (atoms_match(decl->name, name)) {
                found.push(decl);
            }
        }
    }
    return found;
}

internal Ast *ast_alloc(size_t bytes) {
    Ast *memory = (Ast *)push_array(g_ast_arena, u8, bytes);
    return memory;
}

internal Ast_Scope *ast_scope(Scope_Flags flags) {
    Ast_Scope *result = AST_NEW(Ast_Scope);
    result->scope_flags = flags;
    return result;
}

internal Ast_Type_Decl *ast_type_decl(Atom *name, Ast_Type_Info *type_info) {
    Ast_Type_Decl *result = AST_NEW(Ast_Type_Decl);
    result->decl_flags |= DECL_FLAG_TYPE;
    result->name = name;
    result->type_info = type_info;
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

internal Ast_Operator_Proc *ast_operator_proc(Token_Kind op, Auto_Array<Ast_Param*> parameters, Ast_Type_Defn *return_type, Ast_Block *block) {
    Ast_Operator_Proc *result = AST_NEW(Ast_Operator_Proc);
    result->name = atom_create(str8_pushf(g_ast_arena, "operator%s", string_from_token(op)));
    result->op = op;
    result->parameters = parameters;
    result->return_type_defn = return_type;
    result->block = block;
    return result;
}

internal Ast_Struct *ast_struct(Atom *name, Auto_Array<Ast_Struct_Field*> fields) {
    Ast_Struct *result = AST_NEW(Ast_Struct);
    result->decl_flags |= DECL_FLAG_TYPE;
    result->name = name;
    result->fields = fields;
    return result;
}

internal Ast_Struct_Field *ast_struct_field(Atom *name, Ast_Type_Defn *type_defn) {
    Ast_Struct_Field *result = AST_NEW(Ast_Struct_Field);
    result->name = name;
    result->type_defn = type_defn;
    return result;
}

internal Ast_Enum *ast_enum(Atom *name, Auto_Array<Ast_Enum_Field*> fields) {
    Ast_Enum *result = AST_NEW(Ast_Enum);
    result->decl_flags |= DECL_FLAG_TYPE;
    result->name = name;
    result->fields = fields;
    return result;
}

internal Ast_Enum_Field *ast_enum_field(Atom *name) {
    Ast_Enum_Field *result = AST_NEW(Ast_Enum_Field);
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

internal Ast_Type_Info *ast_pointer_type_info(Ast_Type_Info *base) {
    Ast_Type_Info *result = AST_NEW(Ast_Type_Info);
    result->base = base;
    result->type_flags = TYPE_FLAG_POINTER;
    return result;
}

internal Ast_Type_Info *ast_array_type_info(Ast_Type_Info *base) {
    Ast_Type_Info *result = AST_NEW(Ast_Type_Info);
    result->base = base;
    result->type_flags = TYPE_FLAG_ARRAY;
    return result;
}

internal Ast_Proc_Type_Info *ast_proc_type_info(Ast_Type_Info *return_type, Auto_Array<Ast_Type_Info*> parameters) {
    Ast_Proc_Type_Info *result = AST_NEW(Ast_Proc_Type_Info);
    result->type_flags = TYPE_FLAG_PROC;
    result->return_type = return_type;
    result->parameters = parameters;
    return result;
}

internal Ast_Struct_Type_Info *ast_struct_type_info(Auto_Array<Struct_Field_Info> fields) {
    Ast_Struct_Type_Info *result = AST_NEW(Ast_Struct_Type_Info);
    result->type_flags = TYPE_FLAG_STRUCT;
    result->fields = fields;
    result->mem_bytes = 0;
    return result;
}

internal Ast_Enum_Type_Info *ast_enum_type_info(Auto_Array<Enum_Field_Info> fields) {
    Ast_Enum_Type_Info *result = AST_NEW(Ast_Enum_Type_Info);
    result->type_flags = TYPE_FLAG_ENUM;
    result->fields = fields;
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
    result->literal_flags = LITERAL_INT;
    result->int_val = token.intlit;
    return result;
}

internal Ast_Literal *ast_floatlit(Token token) {
    Ast_Literal *result = AST_NEW(Ast_Literal);
    result->literal_flags = LITERAL_FLOAT;
    result->float_val = token.floatlit;
    return result;
}

internal Ast_Literal *ast_strlit(Token token) {
    Ast_Literal *result = AST_NEW(Ast_Literal);
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

internal Ast_Unary *ast_unary_expr(Token op, Ast_Expr *elem) {
    Ast_Unary *result = AST_NEW(Ast_Unary);
    result->mark_start(op.start);
    result->mark_end(elem->end);
    result->op = op;
    result->elem = elem;
    return result;
}

internal Ast_Address *ast_address_expr(Token op, Ast_Expr *elem) {
    Ast_Address *result = AST_NEW(Ast_Address);
    result->mark_start(op.start);
    result->mark_start(elem->end);
    result->elem = elem;
    result->expr_flags |= EXPR_FLAG_LVALUE;
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

internal Ast_Field *ast_field_expr(Ast_Expr *elem, Ast_Field *parent) {
    Ast_Field *result = AST_NEW(Ast_Field);
    if (parent) parent->field_next = result;
    result->elem = elem;
    return result;
}

internal Ast_Expr *ast_error_expr() {
    Ast_Expr *result = AST_NEW(Ast_Expr);
    result->poison();
    return result;
}

internal Ast_Binary *ast_binary_expr(Token op, Ast_Expr *lhs, Ast_Expr *rhs) {
    Ast_Binary *result = AST_NEW(Ast_Binary);
    result->op = op;
    result->lhs = lhs;
    result->rhs = rhs;
    if (lhs) result->mark_start(lhs->start);
    if (rhs) result->mark_end(rhs->end);
    return result; 
}

internal Ast_Binary *ast_arithmetic_expr(Token op, Ast_Expr *lhs, Ast_Expr *rhs) {
    Ast_Binary *result = ast_binary_expr(op, lhs, rhs);
    result->expr_flags = EXPR_FLAG_ARITHMETIC;
    return result;
}

internal Ast_Binary *ast_boolean_expr(Token op, Ast_Expr *lhs, Ast_Expr *rhs) {
    Ast_Binary *result = ast_binary_expr(op, lhs, rhs);
    result->expr_flags = EXPR_FLAG_BOOLEAN;
    return result;
}

internal Ast_Binary *ast_assignment_expr(Token op, Ast_Expr *lhs, Ast_Expr *rhs) {
    Ast_Binary *result = ast_binary_expr(op, lhs, rhs);
    result->expr_flags |= EXPR_FLAG_ASSIGNMENT;
    result->expr_flags |= EXPR_FLAG_LVALUE;
    return result;
}

internal Ast_Binary *ast_comparison_expr(Token op, Ast_Expr *lhs, Ast_Expr *rhs) {
    Ast_Binary *result = ast_binary_expr(op, lhs, rhs);
    result->expr_flags = EXPR_FLAG_COMPARISON;
    return result;
}

internal Ast_Index *ast_index_expr(Token op, Ast_Expr *lhs, Ast_Expr *rhs) {
    Ast_Index *result = AST_NEW(Ast_Index);
    // result->loc = op.l0;
    result->lhs = lhs;
    result->rhs = rhs;
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

internal Ast_For *ast_for_stmt(Ast_Iterator *iterator, Ast_Block *block) {
    Ast_For *result = AST_NEW(Ast_For);
    result->iterator = iterator;
    result->block = block;
    return result;
} 

internal Ast_Return *ast_return(Ast_Expr *expr) {
    Ast_Return *result = AST_NEW(Ast_Return);
    result->expr = expr;
    return result;
}

internal char *string_from_type(Ast_Type_Info *type_info) {
    if (type_info == NULL) return "";
    cstring string = NULL;
    for (Ast_Type_Info *type = type_info; type; type = type->base) {
        if (type->type_flags & TYPE_FLAG_STRUCT) {
            Ast_Struct_Type_Info *struct_type = static_cast<Ast_Struct_Type_Info*>(type);
            string = cstring_append(string, (char *)type->decl->name->data);
        } else if (type->type_flags & TYPE_FLAG_ENUM) {
            Ast_Enum_Type_Info *enum_type = static_cast<Ast_Enum_Type_Info*>(type);
            string = cstring_append(string, (char *)type->decl->name->data);
        } else if (type->type_flags & TYPE_FLAG_PROC) {
            Ast_Proc_Type_Info *proc_type = static_cast<Ast_Proc_Type_Info*>(type);
            string = cstring_append(string, "(");
            for (int i = 0; i < proc_type->parameters.count; i++) {
                Ast_Type_Info *param = proc_type->parameters[i];
                cstring_append(string, string_from_type(param));
                if (i != proc_type->parameters.count - 1) string = cstring_append(string, ",");
            }
            string = cstring_append(string, ")");
            if (proc_type->return_type) {
                string = cstring_append(string, "->(");
                string = cstring_append(string, string_from_type(proc_type->return_type));
                string = cstring_append(string, ")");
            }
        } else if (type->type_flags & TYPE_FLAG_ARRAY) {
            string = cstring_append(string, "[..]");
        } else if (type->type_flags & TYPE_FLAG_POINTER) {
            string = cstring_append(string, "^");
        } else if (type->type_flags & TYPE_FLAG_BUILTIN) {
            string = cstring_append(string, (char *)type->name->data);
        } else {
            Assert(0);
        }
    }
    return string;
}

internal char *string_from_expr(Ast_Expr *expr) {
    if (expr == NULL) return "";
    
    cstring result = NULL;
    switch (expr->kind) {
    case AST_CAST:
    {
        Ast_Cast *cast = static_cast<Ast_Cast*>(expr);
        cstring str = make_cstring("cast(");
        str = cstring_append(str, string_from_type(cast->type_info));
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
    case AST_INDEX:
    {
        Ast_Index *index = (Ast_Index *)expr;
        cstring str = string_from_expr(index->lhs);
        str = cstring_append(str, "[");
        str = cstring_append(str, string_from_expr(index->rhs));
        str = cstring_append(str, "]");
        result = str;
        break;
    }
    case AST_UNARY:
    {
        Ast_Unary *unary = (Ast_Unary *)expr;
        cstring str = string_from_token(unary->op.kind);
        str = cstring_append(str, string_from_expr(unary->elem));
        result = str;
        break;
    }
    case AST_ADDRESS:
    {
        Ast_Address *address = (Ast_Address *)expr;
        cstring str = make_cstring("^");
        str = cstring_append(str, string_from_expr(address->elem));
        result = str;
        break;
    }
    case AST_DEREF:
    {
        Ast_Deref *deref = (Ast_Deref *)expr;
        cstring str = make_cstring("*");
        str = cstring_append(str, string_from_expr(deref->elem));
        result = str;
        break;
    }
    case AST_BINARY:
    {
        Ast_Binary *binary = (Ast_Binary *)expr;
        cstring str = string_from_expr(binary->lhs);
        str = cstring_append(str, string_from_token(binary->op.kind));
        str = cstring_append(str, string_from_expr(binary->rhs));
        result = str;
        break;
    }
    case AST_FIELD:
    {
        Ast_Field *field = (Ast_Field *)expr;
        cstring str = string_from_expr(field->elem);

        //@Todo Instead of going forward to next fields, start from the parent field and go until you reach this field. I think this only works for the parent field expression.
        for (Ast_Field *node = field->field_next; node; node = node->field_next) {
            Assert(node->elem->kind == AST_IDENT);
            Ast_Ident *name = static_cast<Ast_Ident*>(node->elem);
            str = cstring_append(str, ".");
            str = cstring_append(str, (char *)name->name->data);
        }
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
