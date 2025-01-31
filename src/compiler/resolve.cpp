#define ANSI_RESET     "\x1B[0m"
#define ANSI_UNDERLINE "\x1B[4m"
#define ANSI_ITALIC    "\x1B[3m"

Resolver::Resolver(Parser *_parser) {
    arena = arena_alloc(get_virtual_allocator(), MB(4));
    parser = _parser;
}

internal u64 get_next_line_boundary(String8 string, u64 start) {
    for (u64 i = start; i < string.count; i++) {
        if (string.data[i] == '\n' || string.data[i] == '\r') {
            return i;
        }
    }
    return string.count - 1;
} 

void Resolver::error(Ast *elem, const char *fmt, ...) {
    error_count++;
    va_list args;
    va_start(args, fmt);
    String8 string = str8_pushfv(g_error_arena, fmt, args);
    va_end(args);
    printf("%s:%llu:%llu: error: %s", parser->lexer->file_path.data, elem->start.line, elem->start.col, string.data);

    String8 buffer = parser->lexer->file_contents;
    u64 start_index = elem->start.index - elem->start.col;
    u64 line_begin = elem->start.index - elem->start.col;
    u64 line_end = get_next_line_boundary(parser->lexer->file_contents, start_index);

    {
        if (line_begin != elem->start.col) {
            String8 pre = str8(buffer.data + line_begin, elem->start.col);
            printf("\x1B[38;2;168;153;132m");
            printf("%.*s", (int)pre.count, pre.data);
            printf(ANSI_RESET);
        }

        u64 end_index = elem->end.index;
        if (end_index > line_end) end_index = line_end;

        String8 elem_string = str8(buffer.data + elem->start.index, end_index - elem->start.index);
        printf("\x1B[38;2;204;36;29m");
        printf("%.*s", (int)elem_string.count, elem_string.data);
        printf(ANSI_RESET);

        if (line_end > end_index) {
            String8 trailing = str8(buffer.data + end_index, line_end - end_index);
            printf("\x1B[38;2;168;153;132m");
            printf("%.*s", (int)trailing.count, trailing.data);
            printf(ANSI_RESET);

        }

        if (elem->end.index > line_end) {
            printf(ANSI_ITALIC "\x1B[38;2;168;153;132m");
            printf("...");
            printf(ANSI_RESET);
        }
    }

    printf("\n");
    
#if 0
    for (u64 i = start_index; i < elem->start.index; i++) {
        printf(" ");
    }
    u64 stop_index = Min(line_end, elem->end.index);
    for (u64 i = elem->start.index; i < stop_index; i++) {
        printf("-");
    }
    printf("\n");
#endif
}

bool Resolver::in_global_scope() {
    return current_scope == global_scope;
}

void Resolver::add_entry(Ast_Decl *decl) {
    current_scope->declarations.push(decl);
} 

Ast_Decl *Resolver::lookup_local(Atom *name) {
    Ast_Decl *result = NULL;
    Ast_Scope *scope = current_scope;
    for (int i = 0; i < scope->declarations.count; i++) {
        Ast_Decl *decl = scope->declarations[i];
        if (atoms_match(decl->name, name)) {
            result = decl;
            break;
        }
    }
    return result;
}

Ast_Decl *Resolver::lookup(Atom *name) {
    for (Ast_Scope *scope = current_scope; scope; scope = scope->scope_parent) {
        for (int i = 0; i < scope->declarations.count; i++) {
            Ast_Decl *decl = scope->declarations[i];
            if (atoms_match(decl->name, name)) {
                return decl;
            }
        }
    }
    return NULL;
}

Ast_Decl *Resolver::lookup(Ast_Scope *scope, Atom *name) {
    for (int i = 0; i < scope->declarations.count; i++) {
        Ast_Decl *decl = scope->declarations[i];
        if (atoms_match(decl->name, name)) {
            return decl;
        }
    }
    return NULL;
}

Ast_Scope *Resolver::new_scope(Scope_Flags scope_flags) {
    Ast_Scope *parent = current_scope;
    Ast_Scope *scope = ast_scope(scope_flags);
    if (parent) {
        scope->scope_parent = parent;
        scope->level = parent->level + 1;
        DLLPushBack(parent->scope_first, parent->scope_last, scope, scope_next, scope_prev);
    }
    current_scope = scope;
    return scope;
}

void Resolver::exit_scope() {
    current_scope = current_scope->scope_parent;
}

internal Ast_Struct_Field *struct_lookup(Ast_Struct *struct_decl, Atom *name) {
    Ast_Struct_Field *result = NULL;
    for (int i = 0; i < struct_decl->fields.count; i++) {
        Ast_Struct_Field *field = struct_decl->fields[i];
        if (atoms_match(field->name, name)) {
            result = field;
            break;
        }
    }
    return result;
}

internal Ast_Enum_Field *enum_lookup(Ast_Enum *enum_decl, Atom *name) {
    Ast_Enum_Field *result = NULL;
    for (int i = 0; i < enum_decl->fields.count; i++) {
        Ast_Enum_Field *field = enum_decl->fields[i];
        if (atoms_match(field->name, name)) {
            result = field;
            break;
        }
    }
    return result;
}

void Resolver::resolve_if_stmt(Ast_If *if_stmt) {
    Ast_Expr *cond = if_stmt->cond;
    resolve_expr(cond);

    if (cond &&
        cond->valid() &&
        is_struct_type(cond->type_info)) {
        error(cond, "'%s' is not a valid conditional expression.\n", string_from_expr(cond));
    }

    resolve_block(if_stmt->block);
}

void Resolver::resolve_return_stmt(Ast_Return *return_stmt) {
    Ast_Proc_Type_Info *proc_type = static_cast<Ast_Proc_Type_Info*>(current_proc->type_info);

    if (return_stmt->expr) {
        resolve_expr(return_stmt->expr);

        if (proc_type->return_type == type_void) {
            error(return_stmt->expr, "returning value on a void procedure.\n");
            return_stmt->poison();
        }

        if (!typecheck(proc_type->return_type, return_stmt->expr->type_info)) {
            error(return_stmt->expr, "invalid return type '%s', procedure returns '%s'.\n", string_from_type(return_stmt->expr->type_info), string_from_type(proc_type->return_type)); 
            return_stmt->poison();
        }
    } else {
        if (proc_type->return_type != type_void) {
            error(return_stmt, "procedure expects a return type '%s'.\n", string_from_type(proc_type->return_type));
            return_stmt->poison();
        }
    }
}

void Resolver::resolve_decl_stmt(Ast_Decl_Stmt *decl_stmt) {
    Ast_Decl *decl = decl_stmt->decl;

    Ast_Decl *found = lookup_local(decl->name);
    if (found == NULL) {
        if (current_scope->scope_parent && (current_scope->scope_parent->scope_flags & SCOPE_PROC)) {
            found = lookup(current_scope->scope_parent, decl->name);
            if (found) {
                error(decl, "'%s' is already defined as a procedure parameter.\n", decl->name->data);
            }
        }

        resolve_decl(decl_stmt->decl);
        add_entry(decl_stmt->decl);
    } else {
        error(decl, "'%s' already defined.\n", decl->name->data);
        decl->poison();
    }
}

void Resolver::resolve_while_stmt(Ast_While *while_stmt) {
    Ast_Expr *cond = while_stmt->cond;
    resolve_expr(cond);

    if (cond->valid() && !is_conditional_type(cond->type_info)) {
        error(cond, "'%s' is not a conditional expression.\n", string_from_expr(cond));
        while_stmt->poison();
    }

    resolve_block(while_stmt->block);
}

void Resolver::resolve_stmt(Ast_Stmt *stmt) {
    if (stmt == NULL) return;
    
    switch (stmt->kind) {
    case AST_EXPR_STMT:
    {
        Ast_Expr_Stmt *expr_stmt = static_cast<Ast_Expr_Stmt*>(stmt);
        resolve_expr(expr_stmt->expr);
        break;
    }
    case AST_DECL_STMT:
    {
        Ast_Decl_Stmt *decl_stmt = static_cast<Ast_Decl_Stmt*>(stmt);
        resolve_decl_stmt(decl_stmt);
        break;
    }
    case AST_IF:
    {
        Ast_If *if_stmt = static_cast<Ast_If*>(stmt);
        for (Ast_If *node = if_stmt; node; node = node->if_next) {
            resolve_if_stmt(node);
        }
        break;
    }
    case AST_SWITCH:
    {
        break;
    }
    case AST_WHILE:
    {
        Ast_While *while_stmt = static_cast<Ast_While*>(stmt);
        resolve_while_stmt(while_stmt);
        break;
    }
    case AST_FOR:
    {
        break;
    }
    case AST_BLOCK:
    {
        Ast_Block *block = static_cast<Ast_Block*>(stmt);
        resolve_block(block);
        break;
    }
    case AST_RETURN:
    {
        Ast_Return *return_stmt = static_cast<Ast_Return*>(stmt);
        resolve_return_stmt(return_stmt);
        break;
    }
    case AST_GOTO:
    {
        break;
    }
    case AST_DEFER:
    {
        break;
    }
    }
}

void Resolver::resolve_block(Ast_Block *block) {
    Ast_Scope *scope = new_scope(SCOPE_BLOCK);
    scope->block = block;

    block->scope = scope;

    for (int i = 0; i < block->statements.count; i++) {
        Ast_Stmt *stmt = block->statements[i];
        resolve_stmt(stmt);
    }

    exit_scope();
}

Ast_Type_Info *Resolver::resolve_type(Ast_Type_Defn *type_defn) {
    Ast_Type_Info *type = NULL;
    for (Ast_Type_Defn *t = type_defn; t; t = t->base) {
        switch (t->type_defn_kind) {
        case TYPE_DEFN_NIL:
            Assert(0);
            break;
        case TYPE_DEFN_NAME:
        {
            Ast_Decl *decl = lookup(t->name);
            if (decl) {
                resolve_decl(decl);
                if (type) type->base = decl->type_info;
                else type = decl->type_info;
            } else {
                error(t, "undeclared type '%s'.\n", t->name->data);
            }
            break;
        }
        case TYPE_DEFN_POINTER:
        {
            Ast_Type_Info *ptr = ast_pointer_type_info(type);
            type = ptr;
            break;
        }
        case TYPE_DEFN_ARRAY:
        {
            Ast_Type_Info *array = ast_array_type_info(type);
            type = array;
            break;
        }
        }
    }
    return type;
}

void Resolver::resolve_binary_expr(Ast_Binary *binary) {
    Ast_Expr *lhs = binary->lhs;
    Ast_Expr *rhs = binary->rhs;

    resolve_expr(lhs);
    resolve_expr(rhs);

    if (!lhs) {
        lhs = ast_error_expr();
        binary->poison();
    }
    if (!rhs) {
        rhs = ast_error_expr();
        binary->poison();
    }

    if (lhs->invalid()) {
        binary->poison();
    }
    if (rhs->invalid()) {
        binary->poison();
    }

    if (binary->expr_flags & EXPR_FLAG_ASSIGNMENT) {
        if (lhs->valid() && !(lhs->expr_flags & EXPR_FLAG_LVALUE)) {
            error(lhs, "cannot assign to '%s', is not an l-value.\n", string_from_expr(lhs));
        }

        if (lhs->valid() && rhs->valid()) {
            if (!typecheck(lhs->type_info, rhs->type_info)) {
                error(rhs, "cannot assign '%s' to '%s'.\n", string_from_expr(rhs), string_from_expr(lhs));
            }
        }
        binary->type_info = lhs->type_info;
    }

    if (binary->expr_flags & EXPR_FLAG_ARITHMETIC) {
        if (lhs->valid() && !is_arithmetic_type(binary->lhs->type_info)) {
            error(binary->lhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(lhs), string_from_token(binary->op.kind));
        }
        if (rhs->valid() && !is_arithmetic_type(binary->rhs->type_info)) {
            error(binary->rhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(rhs), string_from_token(binary->op.kind));
        }
        binary->type_info = lhs->type_info;
    }

    if (binary->expr_flags & EXPR_FLAG_BOOLEAN) {
        if (lhs->valid() && !is_integral_type(binary->lhs->type_info)) {
            error(binary->lhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(lhs), string_from_token(binary->op.kind));
        }
        if (rhs->valid() && !is_integral_type(binary->rhs->type_info)) {
            error(binary->rhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(rhs), string_from_token(binary->op.kind));
        }
        binary->type_info = lhs->type_info;
    }

    if (binary->expr_flags & EXPR_FLAG_COMPARISON) {
        if (lhs->valid() && !is_arithmetic_type(binary->lhs->type_info)) {
            error(binary->lhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(lhs), string_from_token(binary->op.kind));
        }
        if (rhs->valid() && !is_arithmetic_type(binary->rhs->type_info)) {
            error(binary->rhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(rhs), string_from_token(binary->op.kind));
        }
        binary->type_info = lhs->type_info;
    } 
}

void Resolver::resolve_cast_expr(Ast_Cast *cast) {
    resolve_expr(cast->elem);
    cast->type_info = resolve_type(cast->type_defn);

    if (cast->elem->valid()) {
        if (!typecheck_castable(cast->type_info, cast->elem->type_info)) {
            error(cast->elem, "cannot cast '%s' as '%s' from '%s'.\n", string_from_expr(cast->elem), string_from_type(cast->type_info), string_from_type(cast->elem->type_info));
        }
    } else {
        cast->poison();
    }
}

void Resolver::resolve_index_expr(Ast_Index *index) {
    resolve_expr(index->lhs);
    if (index->lhs->valid() && is_indirection_type(index->lhs->type_info)) {
        index->type_info = deref_type(index->lhs->type_info);
    } else {
        error(index->lhs, "'%s' is not a pointer or array type.\n", string_from_expr(index->lhs));
        index->poison();
    }
}

void Resolver::resolve_compound_literal(Ast_Compound_Literal *literal) {
    Ast_Type_Info *specified_type = resolve_type(literal->type_defn);

    for (int i = 0; i < literal->elements.count; i++) {
        Ast_Expr *elem = literal->elements[i];
        resolve_expr(elem);
    }

    if (is_struct_type(specified_type)) {
        literal->type_info = specified_type;

        Ast_Struct_Type_Info *struct_type = static_cast<Ast_Struct_Type_Info*>(specified_type);
        if (struct_type->fields.count < literal->elements.count) {
            error(literal, "too many initializers for struct '%s'.\n", struct_type->decl->name->data);
            literal->poison();
        }

        int elem_count = Min((int)struct_type->fields.count, (int)literal->elements.count);
        for (int i = 0; i < elem_count; i++) {
            Ast_Expr *elem = literal->elements[i];
            Struct_Field_Info field = struct_type->fields[i];
            if (!typecheck(field.type, elem->type_info)) {
                error(elem, "cannot convert from '%s' to '%s'.\n", string_from_type(elem->type_info), string_from_type(field.type));
                literal->poison();
            }
        }
    } else {
        literal->type_info = ast_array_type_info(specified_type);

        for (int i = 0; i < literal->elements.count; i++) {
            Ast_Expr *elem = literal->elements[i];
            if (!typecheck(specified_type, elem->type_info)) {
                error(elem, "cannot convert from '%s' to '%s'.\n", string_from_type(elem->type_info), string_from_type(specified_type));
                literal->poison();
            }
        }
    }
}

void Resolver::resolve_unary_expr(Ast_Unary *unary) {
    resolve_expr(unary->elem);

    if (unary->elem->invalid()) {
        unary->poison();
    }

    if (unary->elem->valid() && !(unary->elem->type_info->type_flags & TYPE_FLAG_NUMERIC)) {
        error(unary, "invalid operand '%s' of type '%s' in unary '%s'.\n", string_from_expr(unary->elem), string_from_type(unary->elem->type_info), string_from_token(unary->op.kind));
        unary->poison();
    }

    unary->type_info = unary->elem->type_info;
}

void Resolver::resolve_literal(Ast_Literal *literal) {
    if (literal->literal_flags & LITERAL_INT) {
        literal->type_info = type_s32;
    } else if (literal->literal_flags & LITERAL_FLOAT) {
        literal->type_info = type_f32;
    } else if (literal->literal_flags & LITERAL_STRING) {
        // literal->type_info = type_string;
    } else if (literal->literal_flags & LITERAL_BOOLEAN) {
        literal->type_info = type_bool;
    } else {
        Assert(0);
    }
}

void Resolver::resolve_ident(Ast_Ident *ident) {
    Ast_Decl *found = lookup(ident->name);

    if (found) {
        resolve_decl(found);
        ident->type_info = found->type_info;
        if (found->invalid()) ident->poison();
    } else {
        error(ident, "undeclared identifier '%s'.\n", ident->name->data);
        ident->poison();
    }
}

void Resolver::resolve_call_expr(Ast_Call *call) {
    resolve_expr(call->lhs);

    if (call->lhs->valid()) {
        for (int i = 0; i < call->arguments.count; i++) {
            Ast_Expr *arg = call->arguments[i];
            resolve_expr(arg);
            if (arg->invalid()) call->poison();
        }

        if (call->lhs->type_info->type_flags & TYPE_FLAG_PROC) {
            Ast_Proc *proc = static_cast<Ast_Proc*>(call->lhs->type_info->decl);
            if (proc->parameters.count == call->arguments.count) {
                call->type_info = static_cast<Ast_Proc_Type_Info*>(proc->type_info)->return_type;

                for (int i = 0; i < call->arguments.count; i++) {
                    Ast_Param *param = proc->parameters[i];
                    Ast_Expr *arg = call->arguments[i];
                    if (arg->valid() && !typecheck(param->type_info, arg->type_info)) {
                        error(arg, "incompatible argument of type '%s' for parameter of type '%s'.\n", string_from_type(arg->type_info), string_from_type(param->type_info));
                        call->poison();
                    }
                }
            } else {
                error(call, "'%s' does not take %d arguments.\n", string_from_expr(call->lhs), call->arguments.count);
                call->poison();
            }
        } else {
            error(call, "'%s' does not evaluate to a procedure.\n", string_from_expr(call->lhs));
            call->poison();
        }
    }
}

void Resolver::resolve_deref_expr(Ast_Deref *deref) {
    resolve_expr(deref->elem);
    if (is_indirection_type(deref->elem->type_info)) {
        deref->type_info = deref_type(deref->elem->type_info);
    } else {
        error(deref, "cannot dereference '%s', not a pointer type.\n", string_from_expr(deref->elem));
        deref->poison();
    }
}

void Resolver::resolve_field_expr(Ast_Field *field_expr) {
    resolve_expr(field_expr->elem);

    if (field_expr->elem->invalid()) {
        field_expr->poison();
        return;
    }

    Ast_Type_Info *field_expr_type = field_expr->elem->type_info;
    for (Ast_Field *field = field_expr->field_next; field; field = field->field_next) {
        Assert(field->elem->kind == AST_IDENT);
        
        Ast_Ident *name = static_cast<Ast_Ident*>(field->elem);

        Ast_Type_Info *struct_type = NULL;
        Ast_Type_Info *enum_type = NULL;
        if (field_expr_type->type_flags & TYPE_FLAG_POINTER) {
            Ast_Type_Info *type = deref_type(field_expr_type);
            if (type->type_flags & TYPE_FLAG_STRUCT) {
                struct_type = type;
            }
        } else if (field_expr_type->type_flags & TYPE_FLAG_STRUCT) {
            struct_type = field_expr_type;
        } else if (field_expr_type->type_flags & TYPE_FLAG_ENUM) {
            enum_type = field_expr_type;
        }

        if (struct_type) {
            Ast_Struct_Field *struct_field = struct_lookup((Ast_Struct *)struct_type->decl, name->name);
            if (struct_field) {
                field_expr_type = struct_field->type_info;
            } else {
                error(field->elem, "'%s' is not a member of '%s'.\n", name->name->data, struct_type->decl->name->data);
                field_expr->poison();
                break;
            }
        } else if (enum_type) {
            Ast_Enum_Field *enum_field = enum_lookup((Ast_Enum *)enum_type->decl, name->name);
            if (enum_field) {
                field_expr_type = type_s32;
            } else {
                error(field, "'%s' is not a member of '%s'.\n", name->name->data, enum_type->decl->name->data);
                field_expr->poison();
                break;
            }
        } else {
            error(field->field_prev, "'%s' is not a struct, enum, or pointer to struct type.\n", string_from_expr(field->field_prev)); 
            field_expr->poison();
            break;
        }
    }

    field_expr->type_info = field_expr_type;
}

void Resolver::resolve_range_expr(Ast_Range *range) {
    resolve_expr(range->lhs);
    resolve_expr(range->rhs);

    if (range->lhs->valid()) {
        if (!is_integral_type(range->lhs->type_info)) {
            error(range->lhs, "'%s' is invalid range expression, not an integral type.\n", string_from_expr(range->lhs));
            range->poison();
        }
    }

    if (range->rhs->valid()) {
        if (!is_integral_type(range->rhs->type_info)) {
            error(range->rhs, "'%s' is invalid range expression, not an integral type.\n", string_from_expr(range->rhs));
            range->poison();
        }
    }
}

void Resolver::resolve_address_expr(Ast_Address *address) {
    resolve_expr(address->elem);

    if (address->elem->valid()) {
        address->type_info = ast_pointer_type_info(address->elem->type_info);

        if (!(address->elem->expr_flags & EXPR_FLAG_LVALUE)) {
            error(address->elem, "cannot take address of '%s'.\n", string_from_expr(address->elem));
            address->poison();
        }
    } else {
        address->poison();
    }
}

void Resolver::resolve_expr(Ast_Expr *expr) {
    if (expr == NULL) return;

    if (expr->visited) return;

    switch (expr->kind) {
    default:
        Assert(0);
        break;

    case AST_PAREN:
    {
        Ast_Paren *paren = static_cast<Ast_Paren*>(expr);
        resolve_expr(paren->elem);
        paren->type_info = paren->elem->type_info;

        if (paren->elem->invalid()) {
            paren->poison();
        }
        break;
    }

    case AST_LITERAL:
    {
        Ast_Literal *literal = static_cast<Ast_Literal *>(expr);
        resolve_literal(literal);
        break;
    }

    case AST_COMPOUND_LITERAL:
    {
        Ast_Compound_Literal *literal = static_cast<Ast_Compound_Literal*>(expr);
        resolve_compound_literal(literal);
        break;
    }

    case AST_IDENT:
    {
        Ast_Ident *ident = static_cast<Ast_Ident*>(expr);
        resolve_ident(ident);
        break;
    }

    case AST_CALL:
    {
        Ast_Call *call = static_cast<Ast_Call*>(expr);
        resolve_call_expr(call);
        break;
    }
        
    case AST_INDEX:
    {
        Ast_Index *index = static_cast<Ast_Index *>(expr);
        resolve_index_expr(index);
        break;
    }

    case AST_CAST:
    {
        Ast_Cast *cast = static_cast<Ast_Cast*>(expr);
        resolve_cast_expr(cast);
        break;
    }

    case AST_UNARY:
    {
        Ast_Unary *unary = static_cast<Ast_Unary*>(expr);
        resolve_unary_expr(unary);
        break;
    }

    case AST_ADDRESS:
    {
        Ast_Address *address = static_cast<Ast_Address*>(expr);
        resolve_address_expr(address);
        break;
    }

    case AST_DEREF:
    {
        Ast_Deref *deref = static_cast<Ast_Deref*>(expr);
        resolve_deref_expr(deref);
        break;
    }

    case AST_BINARY:
    {
        Ast_Binary *binary = static_cast<Ast_Binary*>(expr);
        resolve_binary_expr(binary);
        break;
    }

    case AST_FIELD:
    {
        Ast_Field *field = static_cast<Ast_Field*>(expr);
        resolve_field_expr(field);
        break;
    }

    case AST_RANGE:
    {
        Ast_Range *range = static_cast<Ast_Range*>(expr);
        resolve_range_expr(range);
        break;
    }

    case AST_TERNARY:
    {
        break;
    }
    }

    expr->visited = true;
}

void Resolver::resolve_proc_header(Ast_Proc *proc) {
    Ast_Type_Info *return_type = resolve_type(proc->return_type_defn);
    Auto_Array<Ast_Type_Info*> parameters;
    for (int i = 0; i < proc->parameters.count; i++) {
        Ast_Param *param = proc->parameters[i];
        Ast_Type_Info *type = resolve_type(param->type_defn);
        parameters.push(type);
    }
    if (return_type == NULL) return_type = type_void;

    Ast_Proc_Type_Info *type = ast_proc_type_info(return_type, parameters);
    type->decl = proc;
    proc->type_info = type;
}

void Resolver::resolve_control_path_flow(Ast_Proc *proc) {
    Ast_Block *block = proc->block;
    for (int i = 0; i < block->statements.count; i++) {
        Ast_Stmt *stmt = block->statements[i];
        if (stmt->kind == AST_IF) {
            Ast_If *if_stmt = static_cast<Ast_If*>(stmt);
            if_stmt->block->block_parent = block;
            for (Ast_If *node = if_stmt; node; node = node->if_next) {
                DLLPushBack(block->block_first, block->block_last, if_stmt->block, block_next, block_prev);
            }
        }

    }
}

void Resolver::resolve_proc(Ast_Proc *proc) {
    resolve_proc_header(proc);
    if (in_global_scope()) {
        Ast_Scope *scope = new_scope(SCOPE_PROC);
        proc->scope = scope;
        current_proc = proc;
        for (int i = 0; i < proc->parameters.count; i++) {
            Ast_Param *param = proc->parameters[i];
            add_entry(param);
        }
        resolve_block(proc->block);
        exit_scope();
    }
    // resolve_control_path_flow(proc);
}

void Resolver::resolve_struct(Ast_Struct *struct_decl) {
    Auto_Array<Struct_Field_Info> struct_fields;
    for (int i = 0; i < struct_decl->fields.count; i++) {
        Ast_Struct_Field *field = struct_decl->fields[i];
        field->type_info = resolve_type(field->type_defn);

        Struct_Field_Info field_info = {};
        field_info.type = field->type_info;
        field_info.mem_offset = 0;
        struct_fields.push(field_info);
    }
    Ast_Struct_Type_Info *type_info = ast_struct_type_info(struct_fields);
    struct_decl->type_info = type_info;
    type_info->decl = struct_decl;
    type_info->name = struct_decl->name;
}

void Resolver::resolve_enum(Ast_Enum *enum_decl) {
    Auto_Array<Enum_Field_Info> enum_fields;
    for (int i = 0; i < enum_decl->fields.count; i++) {
        Ast_Enum_Field *field = enum_decl->fields[i];

        Enum_Field_Info field_info = {};
        field_info.name = field->name;
        enum_fields.push(field_info);
    }

    Ast_Enum_Type_Info *type_info = ast_enum_type_info(enum_fields);
    enum_decl->type_info = type_info;
    type_info->decl = enum_decl;
    type_info->name = enum_decl->name;
}

void Resolver::resolve_var(Ast_Var *var) {
    Ast_Type_Info *specified_type = NULL;
    if (var->type_defn) {
        specified_type = resolve_type(var->type_defn);
    }
    if (var->init) {
        resolve_expr(var->init);
        if (var->init->invalid()) var->poison();
    }

    if (var->type_defn && var->init) {
        var->type_info = specified_type;
        if (var->init->valid() && !typecheck(specified_type, var->init->type_info)) {
            error(var->init, "cannot assign '%s' of type '%s' to '%s'\n", string_from_expr(var->init), string_from_type(var->init->type_info), var->name->data);
            var->poison();
        }
    } else if (var->type_defn != NULL) {
        var->type_info = specified_type;
    } else {
        var->type_info = var->init->type_info;
    }
}

void Resolver::resolve_param(Ast_Param *param) {
    Ast_Type_Info *type_info = resolve_type(param->type_defn);
    param->type_info = type_info;
}

void Resolver::register_global_declarations() {
    Ast_Root *root = parser->root;

    global_scope = new_scope(SCOPE_GLOBAL);
    root->scope = global_scope;

    for (Builtin_Type_Kind builtin_type = BUILTIN_TYPE_VOID; builtin_type < BUILTIN_TYPE_COUNT; builtin_type = (Builtin_Type_Kind)(builtin_type + 1)) {
        Ast_Type_Info *type_info = g_builtin_types[builtin_type];
        Ast_Type_Decl *decl = ast_type_decl(type_info->name, type_info);
        global_scope->declarations.push(decl);
    }

    for (int i = 0; i < root->declarations.count; i++) {
        Ast_Decl *decl = root->declarations[i];
        Ast_Decl *found = lookup(decl->name);
        if (found == NULL) {
            add_entry(decl);
        } else {
            error(decl, "declaration '%s' already named.\n", (char *)decl->name->data);
        }
    }
}

void Resolver::resolve_decl(Ast_Decl *decl) {
    if (decl->resolve_state == RESOLVE_DONE) {
        return;
    } else if (decl->resolve_state == RESOLVE_STARTED &&
        decl != current_proc) {
        error(decl, "cyclical resolution.\n");
        Assert(0);
        return;
    }

    bool incomplete_resolve = false;
    decl->resolve_state = RESOLVE_STARTED;

    switch (decl->kind) {
    case AST_VAR:
    {
        Ast_Var *var = static_cast<Ast_Var*>(decl);
        resolve_var(var);
        break;
    }
    case AST_PARAM:
    {
        Ast_Param *param = static_cast<Ast_Param*>(decl);
        resolve_param(param);
        break;
    }
    case AST_PROC:
    {
        Ast_Proc *proc = static_cast<Ast_Proc*>(decl);
        resolve_proc(proc);
        if (in_global_scope()) {
            incomplete_resolve = true;
        }
        break;
    }
    case AST_STRUCT:
    {
        Ast_Struct *struct_decl = static_cast<Ast_Struct*>(decl);
        resolve_struct(struct_decl);
        break;
    }
    case AST_ENUM:
    {
        Ast_Enum *enum_decl = static_cast<Ast_Enum*>(decl);
        resolve_enum(enum_decl);
        break;
    }
    }

    decl->resolve_state = RESOLVE_DONE;
    if (incomplete_resolve) {
        decl->resolve_state = RESOLVE_UNSTARTED;
    }
}

void Resolver::resolve() {
    register_global_declarations();
    
    Ast_Root *root = parser->root;
    for (int i = 0; i < root->declarations.count; i++) {
        Ast_Decl *decl = root->declarations[i];
        resolve_decl(decl);
    }
}
