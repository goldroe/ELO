#include <unordered_set>

Resolver::Resolver(Parser *_parser) {
    arena = arena_create();
    parser = _parser;
}

bool Resolver::in_global_scope() {
    return current_scope == global_scope;
}

void Resolver::add_entry(Ast_Decl *decl) {
    current_scope->declarations.push(decl);
} 

Ast_Decl *Resolver::lookup_local(Atom *name) {
    Ast_Decl *result = NULL;
    Scope *scope = current_scope;
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
    for (Scope *scope = current_scope; scope; scope = scope->parent) {
        for (int i = 0; i < scope->declarations.count; i++) {
            Ast_Decl *decl = scope->declarations[i];
            if (atoms_match(decl->name, name)) {
                return decl;
            }
        }
    }
    return NULL;
}

Ast_Decl *Resolver::lookup(Scope *scope, Atom *name) {
    for (int i = 0; i < scope->declarations.count; i++) {
        Ast_Decl *decl = scope->declarations[i];
        if (atoms_match(decl->name, name)) {
            return decl;
        }
    }
    return NULL;
}

Ast_Operator_Proc *Resolver::lookup_user_defined_binary_operator(Token_Kind op, Ast_Type_Info *lhs, Ast_Type_Info *rhs) {
    Ast_Operator_Proc *result = NULL;
    Scope *scope = global_scope;
    for (int i = 0; i < scope->declarations.count; i++) {
        Ast_Decl *decl = scope->declarations[i];
        if (decl->kind == AST_OPERATOR_PROC) {
            Ast_Operator_Proc *proc = static_cast<Ast_Operator_Proc*>(decl);
            resolve_proc_header(proc);
            Ast_Proc_Type_Info *proc_type = static_cast<Ast_Proc_Type_Info*>(proc->type_info);
            if (proc->op == op && proc_type->parameters.count == 2 &&
                typecheck(proc_type->parameters[0], lhs) &&
                typecheck(proc_type->parameters[1], rhs)) {
                result = proc;
                break;
            }
        }
    }
    return result;
}

Ast_Operator_Proc *Resolver::lookup_user_defined_unary_operator(Token_Kind op, Ast_Type_Info *type) {
    Ast_Operator_Proc *result = NULL;
    Scope *scope = global_scope;
    for (int i = 0; i < scope->declarations.count; i++) {
        Ast_Decl *decl = scope->declarations[i];
        if (decl->kind == AST_OPERATOR_PROC) {
            Ast_Operator_Proc *proc = static_cast<Ast_Operator_Proc*>(decl);
            resolve_proc_header(proc);
            Ast_Proc_Type_Info *proc_type = static_cast<Ast_Proc_Type_Info*>(proc->type_info);
            if (proc->op == op && proc_type->parameters.count == 1 &&
                typecheck(proc_type->parameters[0], type)) {
                result = proc;
                break;
            }
        }
    }
    return result;
}

Ast_Decl *Resolver::lookup_overloaded(Atom *name, Auto_Array<Ast_Expr*> arguments, bool *overloaded) {
    Ast_Proc *result = NULL;

    //@Note Check local scopes first and return first
    for (Scope *scope = current_scope; scope->parent != NULL; scope = scope->parent) {
        for (int i = 0; i < scope->declarations.count; i++) {
            Ast_Decl *decl = scope->declarations[i];
            if (atoms_match(name, decl->name)) {
                *overloaded = false;
                return decl;
            }
        }
    }

    //@Note Check global scope for all possible overloaded procedures
    Auto_Array<Ast_Decl*> candidates;
    for (int i = 0; i < global_scope->declarations.count; i++) {
        Ast_Decl *decl = global_scope->declarations[i];
        if (atoms_match(decl->name, name)) {
            candidates.push(decl);
        }
    }

    if (candidates.count == 1) {
        *overloaded = false;
        return candidates[0];
    }

    for (int i = 0; i < candidates.count; i++) {
        *overloaded = true;

        Ast_Decl *decl = candidates[i];
        if (decl->kind == AST_PROC) {
            Ast_Proc *proc = static_cast<Ast_Proc*>(decl);
            resolve_proc_header(proc);
            Ast_Proc_Type_Info *proc_type = static_cast<Ast_Proc_Type_Info*>(proc->type_info);
            if (proc_type->parameters.count == arguments.count) {
                bool match = true;
                for (int j = 0; j < arguments.count; j++) {
                    Ast_Type_Info *param = proc_type->parameters[j];
                    Ast_Expr *arg = arguments[j];
                    if (!typecheck(param, arg->type_info)) {
                        match = false;
                        break;
                    }
                }

                if (match) {
                    result = proc;
                    break;
                }
            }
        }
    }
    return result;
}

Scope *Resolver::new_scope(Scope_Flags scope_flags) {
    Scope *parent = current_scope;
    Scope *scope = make_scope(scope_flags);
    if (parent) {
        scope->parent = parent;
        scope->level = parent->level + 1;
        DLLPushBack(parent->first, parent->last, scope, next, prev);
    }
    current_scope = scope;
    return scope;
}

void Resolver::exit_scope() {
    current_scope = current_scope->parent;
}

internal Struct_Field_Info *struct_lookup(Ast_Type_Info *type_info, Atom *name) {
    for (int i = 0; i < type_info->aggregate.fields.count; i++) {
        Struct_Field_Info *field = &type_info->aggregate.fields[i];
        if (atoms_match(field->name, name)) {
            return field;
        }
    }
    return NULL;
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
        cond->type_info->is_struct_type()) {
        report_ast_error(cond, "'%s' is not a valid conditional expression.\n", string_from_expr(cond));
    }

    resolve_block(if_stmt->block);
}

void Resolver::resolve_return_stmt(Ast_Return *return_stmt) {
    Ast_Proc_Type_Info *proc_type = static_cast<Ast_Proc_Type_Info*>(current_proc->type_info);
    current_proc->returns = true;

    if (return_stmt->expr) {
        resolve_expr(return_stmt->expr);

        if (proc_type->return_type == type_void) {
            report_ast_error(return_stmt->expr, "returning value on a void procedure.\n");
            return_stmt->poison();
        }

        if (return_stmt->expr->valid() &&
            !typecheck(proc_type->return_type, return_stmt->expr->type_info)) {
            report_ast_error(return_stmt->expr, "invalid return type '%s', procedure returns '%s'.\n", string_from_type(return_stmt->expr->type_info), string_from_type(proc_type->return_type)); 
            return_stmt->poison();
        }
    } else {
        if (proc_type->return_type != type_void) {
            report_ast_error(return_stmt, "procedure expects a return type '%s'.\n", string_from_type(proc_type->return_type));
            return_stmt->poison();
        }
    }
}

void Resolver::resolve_break_stmt(Ast_Break *break_stmt) {
    if (breakcont_stack.count > 0) {
        break_stmt->target = breakcont_stack.back();
    } else {
        report_ast_error(break_stmt, "illegal break.\n");
    }
}

void Resolver::resolve_continue_stmt(Ast_Continue *continue_stmt) {
    if (breakcont_stack.count > 0) {
        Ast *target = breakcont_stack.back();
        if (target->kind == AST_IFCASE) {
            report_ast_error(continue_stmt, "illegal continue.\n");
        }
        continue_stmt->target = target;
    } else {
        report_ast_error(continue_stmt, "illegal continue.\n");
    }
}

void Resolver::resolve_decl_stmt(Ast_Decl_Stmt *decl_stmt) {
    Ast_Decl *decl = decl_stmt->decl;

    Ast_Decl *found = lookup_local(decl->name);
    if (found == NULL) {
        if (current_scope->parent && (current_scope->parent->scope_flags & SCOPE_PROC)) {
            found = lookup(current_scope->parent, decl->name);
            if (found) {
                report_redeclaration(decl);
            }
        }

        resolve_decl(decl_stmt->decl);
        add_entry(decl_stmt->decl);
    } else {
        report_redeclaration(decl);
        decl->poison();
    }
}

void Resolver::resolve_while_stmt(Ast_While *while_stmt) {
    Ast_Expr *cond = while_stmt->cond;
    resolve_expr(cond);

    if (cond->valid() && !cond->type_info->is_conditional_type()) {
        report_ast_error(cond, "'%s' is not a conditional expression.\n", string_from_expr(cond));
        while_stmt->poison();
    }

    breakcont_stack.push(while_stmt);
    resolve_block(while_stmt->block);
    breakcont_stack.pop();
}

void Resolver::resolve_for_stmt(Ast_For *for_stmt) {
    Scope *scope = new_scope(SCOPE_BLOCK);

    add_entry(for_stmt->var);
    resolve_var(for_stmt->var);

    resolve_expr(for_stmt->iterator);

    breakcont_stack.push(for_stmt);
    resolve_block(for_stmt->block);
    breakcont_stack.pop();

    exit_scope();
}

void Resolver::resolve_ifcase_stmt(Ast_Ifcase *ifcase) {
    resolve_expr(ifcase->cond);

    ifcase->switch_jumptable = true;

    if (ifcase->cond) {
        if (!ifcase->cond->type_info->is_integral_type()) {
            ifcase->switch_jumptable = false;
        }
    } else {
        ifcase->switch_jumptable = false;
    }

    if (ifcase->cases.count == 0) {
        report_ast_error(ifcase, "case statement contains no cases.\n");
        ifcase->poison();
    }


    breakcont_stack.push(ifcase);

    for (Ast_Case_Label *case_label : ifcase->cases) {
        case_label->scope = new_scope(SCOPE_BLOCK);

        if (case_label->cond) {
            resolve_expr(case_label->cond);
            if (!case_label->cond->is_constant()) {
                ifcase->switch_jumptable = false;
            }
        } else {
            case_label->is_default = true;
            if (ifcase->default_case) {
                report_ast_error(case_label, "multiple defaults in case statement.\n");
            } else {
                ifcase->default_case = case_label;
            }
        }
            
        if (ifcase->cond) {
            if (case_label->cond) {
                if (!typecheck(ifcase->cond->type_info, case_label->cond->type_info)) {
                    report_ast_error(case_label->cond, "'%s' is illegal type for case expression.\n", string_from_type(case_label->cond->type_info));
                    ifcase->poison();
                }
            }
        } else {
            if (case_label->cond->kind == AST_RANGE) {
                //@Todo report crashes
                report_ast_error(case_label, "illegal range condition in an ifcase without an initial condition to compare.\n");
                ifcase->poison();
            }
        }

        for (Ast_Stmt *s : case_label->statements) {
            resolve_stmt(s);
        }
        exit_scope();
    }

    breakcont_stack.pop();

    if (ifcase->switch_jumptable) {
        std::unordered_set<u64> switch_constants;

        for (Ast_Case_Label *label : ifcase->cases) {
            if (label->cond) {
                if (label->cond->kind == AST_RANGE) {
                    Ast_Range *range = static_cast<Ast_Range*>(label->cond);
                    u64 min = range->lhs->eval.int_val;
                    u64 max = range->rhs->eval.int_val;
                    for (u64 c = min; c <= max; c++) {
                        auto find = switch_constants.find(c);
                        if (find != switch_constants.end()) {
                            ifcase->poison();
                            report_ast_error(label, "case value '%llu' in range '%s' already used.\n", c, string_from_expr(label->cond));
                            break;
                        } else {
                            switch_constants.insert(c);
                        }
                    }
                } else {
                    u64 c = label->cond->eval.int_val;
                    auto find = switch_constants.find(c);
                    if (find != switch_constants.end()) {
                        report_ast_error(label, "case value '%llu' already used.\n", c);
                        ifcase->poison();
                    } else {
                        switch_constants.insert(c);
                    }
                }
            }
        }
    }
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

    case AST_IFCASE:
    {
        Ast_Ifcase *ifcase = static_cast<Ast_Ifcase*>(stmt);
        resolve_ifcase_stmt(ifcase);
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

    case AST_WHILE:
    {
        Ast_While *while_stmt = static_cast<Ast_While*>(stmt);
        resolve_while_stmt(while_stmt);
        break;
    }
    case AST_FOR:
    {
        Ast_For *for_stmt = static_cast<Ast_For*>(stmt);
        resolve_for_stmt(for_stmt);
        break;
    }
    case AST_BLOCK:
    {
        Ast_Block *block = static_cast<Ast_Block*>(stmt);
        resolve_block(block);
        break;
    }
    case AST_BREAK:
    {
        Ast_Break *break_stmt = static_cast<Ast_Break*>(stmt);
        resolve_break_stmt(break_stmt);
        break;
    }
    case AST_CONTINUE:
    {
        Ast_Continue *continue_stmt = static_cast<Ast_Continue*>(stmt);
        resolve_continue_stmt(continue_stmt);
        break;
    }
    case AST_RETURN:
    {
        Ast_Return *return_stmt = static_cast<Ast_Return*>(stmt);
        resolve_return_stmt(return_stmt);
        break;
    }
    }
}

void Resolver::resolve_block(Ast_Block *block) {
    Scope *scope = new_scope(SCOPE_BLOCK);
    scope->block = block;
    block->scope = scope;

    for (Ast_Stmt *stmt : block->statements) {
        resolve_stmt(stmt);
        if (stmt->kind == AST_BLOCK) {
            Ast_Block *b = static_cast<Ast_Block*>(stmt);
            if (b->returns) block->returns = true;
        } else if (stmt->kind == AST_RETURN) block->returns = true;
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
                report_ast_error(t, "undeclared type '%s'.\n", t->name->data);
                return type_poison;
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
            Ast_Expr *array_size = t->array_size;
            Ast_Array_Type_Info *array = ast_array_type_info(type);

            if (array_size) {
                resolve_expr(array_size);
                if (array_size->type_info->is_integral_type()) {
                    if (array_size->is_constant()) {
                        array->array_size = array_size->eval.int_val;
                        array->is_fixed = true;
                    } else {
                        array->is_dynamic = true;
                    }
                } else {
                    report_ast_error(array_size, "array size is not of integral type.\n");
                    return type_poison;
                }
            }

            type = array;
            break;
        }
        }
    }
    return type;
}

void Resolver::resolve_builtin_operator_expr(Ast_Binary *binary) {
    Ast_Expr *lhs = binary->lhs;
    Ast_Expr *rhs = binary->rhs;

    Assert(lhs->valid() && rhs->valid());
    
    if (binary->expr_flags & EXPR_FLAG_ARITHMETIC) {
        if (!lhs->type_info->is_arithmetic_type()) {
            report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(lhs), string_from_token(binary->op.kind));
            binary->poison();
        }
        if (!rhs->type_info->is_arithmetic_type()) {
            report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(rhs), string_from_token(binary->op.kind));
            binary->poison();
        }
        binary->type_info = lhs->type_info;
    }

    if (binary->expr_flags & EXPR_FLAG_BOOLEAN) {
        if (!lhs->type_info->is_integral_type()) {
            report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(lhs), string_from_token(binary->op.kind));
            binary->poison();
        }
        if (!rhs->type_info->is_integral_type()) {
            report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(rhs), string_from_token(binary->op.kind));
            binary->poison();
        }
        binary->type_info = type_bool;
    }

    if (binary->expr_flags & EXPR_FLAG_COMPARISON) {
        //@Note Give null expr type of lhs
        if (rhs->kind == AST_NULL) {
            rhs->type_info = lhs->type_info;
        }
        if (!lhs->type_info->is_arithmetic_type()) {
            report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(lhs), string_from_token(binary->op.kind));
            binary->poison();
        }
        if (!rhs->type_info->is_arithmetic_type()) {
            report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(rhs), string_from_token(binary->op.kind));
            binary->poison();
        }
        binary->type_info = type_bool;
    }

    if (binary->valid() &&
        lhs->is_constant() && rhs->is_constant()) {
        binary->expr_flags |= EXPR_FLAG_CONSTANT;
        binary->eval = eval_binary_expr(binary);
    }
}

Eval Resolver::eval_unary_expr(Ast_Unary *u) {
    Eval result = {};
    Eval e = u->elem->eval;
    if (u->elem->type_info->is_integral_type()) {
        switch (u->op.kind) {
        case TOKEN_PLUS:
            result.int_val = e.int_val;
        case TOKEN_MINUS:
            result.int_val = e.int_val;
        case TOKEN_BANG:
            result.int_val = !e.int_val;
        }
    } else if (u->elem->type_info->is_float_type()) {
        switch (u->op.kind) {
        case TOKEN_PLUS:
            result.float_val = e.float_val;
        case TOKEN_MINUS:
            result.float_val = -e.float_val;
        case TOKEN_BANG:
            result.float_val = !e.float_val;
        }
    }
    return result;
}

Eval Resolver::eval_binary_expr(Ast_Binary *b) {
    Eval result = {};
    Eval lhs = b->lhs->eval;
    Eval rhs = b->rhs->eval;    

    if (b->lhs->type_info->is_integral_type()) {
        switch (b->op.kind) {
        case TOKEN_PLUS:
            result.int_val = lhs.int_val + rhs.int_val;
            break;
        case TOKEN_MINUS:
            result.int_val = lhs.int_val - rhs.int_val;
            break;
        case TOKEN_STAR:
            result.int_val = lhs.int_val * rhs.int_val;
            break;
        case TOKEN_SLASH:
            result.int_val = lhs.int_val / rhs.int_val;
            break;
        case TOKEN_MOD:
            result.int_val = lhs.int_val % rhs.int_val;
            break;
        case TOKEN_NEQ:
            result.int_val = lhs.int_val != rhs.int_val;
            break;
        case TOKEN_LT:
            result.int_val = lhs.int_val < rhs.int_val;
            break;
        case TOKEN_LTEQ:
            result.int_val = lhs.int_val <= rhs.int_val;
            break;
        case TOKEN_GT:
            result.int_val = lhs.int_val > rhs.int_val;
            break;
        case TOKEN_GTEQ:
            result.int_val = lhs.int_val >= rhs.int_val;
            break;
        case TOKEN_AMPER:
            result.int_val = lhs.int_val & rhs.int_val;
            break;
        case TOKEN_AND:
            result.int_val = lhs.int_val && rhs.int_val;
            break;
        case TOKEN_BAR:
            result.int_val = lhs.int_val | rhs.int_val;
            break;
        case TOKEN_OR:
            result.int_val = lhs.int_val || rhs.int_val;
            break;
        case TOKEN_XOR:
            result.int_val = lhs.int_val ^ rhs.int_val;
            break;
        }
    }    
    
    if (b->lhs->type_info->is_float_type()) {
        switch (b->op.kind) {
        case TOKEN_PLUS:
            result.float_val = lhs.float_val + rhs.float_val;
            break;
        case TOKEN_MINUS:
            result.float_val = lhs.float_val - rhs.float_val;
            break;
        case TOKEN_STAR:
            result.float_val = lhs.float_val * rhs.float_val;
            break;
        case TOKEN_SLASH:
            result.float_val = lhs.float_val / rhs.float_val;
            break;
        case TOKEN_MOD:
            result.float_val = fmod(lhs.float_val, rhs.float_val);
            break;
        case TOKEN_NEQ:
            result.float_val = lhs.float_val != rhs.float_val;
            break;
        case TOKEN_LT:
            result.float_val = lhs.float_val < rhs.float_val;
            break;
        case TOKEN_LTEQ:
            result.float_val = lhs.float_val <= rhs.float_val;
            break;
        case TOKEN_GT:
            result.float_val = lhs.float_val > rhs.float_val;
            break;
        case TOKEN_GTEQ:
            result.float_val = lhs.float_val >= rhs.float_val;
            break;
        case TOKEN_AND:
            result.float_val = lhs.float_val && rhs.float_val;
            break;
        case TOKEN_OR:
            result.float_val = lhs.float_val || rhs.float_val;
            break;
        }        
    }

    return result;
}

void Resolver::resolve_user_defined_operator_expr(Ast_Binary *expr) {
    Token op = expr->op;
    Ast_Operator_Proc *proc = lookup_user_defined_binary_operator(op.kind, expr->lhs->type_info, expr->rhs->type_info);
    if (proc) {
        resolve_proc_header(proc);
        Ast_Proc_Type_Info *proc_type = static_cast<Ast_Proc_Type_Info*>(proc->type_info);
        expr->type_info = proc_type->return_type;
        expr->proc = proc;
    } else {
        report_ast_error(expr, "no binary operator'%s' (%s,%s) found.\n", string_from_token(op.kind), string_from_type(expr->lhs->type_info), string_from_type(expr->rhs->type_info));
        expr->poison();
    }
}

void Resolver::resolve_user_defined_operator_expr(Ast_Unary *expr) {
    Token op = expr->op;
    Ast_Operator_Proc *proc = lookup_user_defined_unary_operator(op.kind, expr->elem->type_info);
    if (proc) {
        expr->type_info = static_cast<Ast_Proc_Type_Info*>(proc->type_info)->return_type;
        expr->proc = proc;
    } else {
        report_ast_error(expr, "no unary operator'%s' (%s) found.\n", string_from_token(op.kind), string_from_type(expr->elem->type_info));
        expr->poison();
    }
}

void Resolver::resolve_assignment_expr(Ast_Assignment *assignment) {
    Ast_Expr *lhs = assignment->lhs;
    Ast_Expr *rhs = assignment->rhs;

    resolve_expr(lhs);
    resolve_expr(rhs);

    //@Note Give null expr type of lhs
    if (rhs->kind == AST_NULL) {
        rhs->type_info = lhs->type_info;
    }

    if (lhs->valid() && !(lhs->expr_flags & EXPR_FLAG_LVALUE)) {
        report_ast_error(lhs, "cannot assign to '%s', is not an l-value.\n", string_from_expr(lhs));
        assignment->poison();
    }

    if (lhs->valid() && rhs->valid()) {
        if (!typecheck(lhs->type_info, rhs->type_info)) {
            report_ast_error(rhs, "type mismatch, cannot assign to '%s' from '%s'.\n", string_from_type(lhs->type_info), string_from_type(rhs->type_info));
            assignment->poison();
        }
    }
    assignment->type_info = lhs->type_info;
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

    if (lhs->valid() && rhs->valid()) {
        if (!lhs->type_info->is_custom_type() && !rhs->type_info->is_custom_type()) {
            resolve_builtin_operator_expr(binary);  
        } else {
            binary->expr_flags |= EXPR_FLAG_OP_CALL;
            resolve_user_defined_operator_expr(binary);
        }
    }
}

void Resolver::resolve_cast_expr(Ast_Cast *cast) {
    resolve_expr(cast->elem);
    cast->type_info = resolve_type(cast->type_defn);

    if (cast->elem->valid()) {
        if (!typecheck_castable(cast->type_info, cast->elem->type_info)) {
            report_ast_error(cast->elem, "cannot cast '%s' as '%s' from '%s'.\n", string_from_expr(cast->elem), string_from_type(cast->type_info), string_from_type(cast->elem->type_info));
        }
    } else {
        cast->poison();
    }
}

void Resolver::resolve_subscript_expr(Ast_Subscript *subscript) {
    resolve_expr(subscript->expr);
    resolve_expr(subscript->index);

    if (subscript->expr->valid()) {
        if (subscript->expr->type_info->is_indirection_type()) {
            subscript->type_info = subscript->expr->type_info->deref();
        } else {
            report_ast_error(subscript->expr, "'%s' is not a pointer or array type.\n", string_from_expr(subscript->expr));
            subscript->poison();
        }
    } else {
        subscript->poison();
    }

    if (subscript->index->valid()) {
        if (!subscript->index->type_info->is_integral_type()) {
            report_ast_error(subscript->index, "array subscript is not of integral type.\n");
            subscript->poison();
        }
    } else {
        subscript->poison();
    }
}

void Resolver::resolve_compound_literal(Ast_Compound_Literal *literal) {
    Ast_Type_Info *specified_type = resolve_type(literal->type_defn);

    literal->type_info = specified_type;

    for (int i = 0; i < literal->elements.count; i++) {
        Ast_Expr *elem = literal->elements[i];
        resolve_expr(elem);

    }

    if (specified_type->is_struct_type()) {
        Ast_Type_Info *struct_type = specified_type;
        if (struct_type->aggregate.fields.count < literal->elements.count) {
            report_ast_error(literal, "too many initializers for struct '%s'.\n", struct_type->decl->name->data);
            report_note(struct_type->decl->start, literal->file, "see declaration of '%s'.\n", struct_type->decl->name->data);
            literal->poison();
        }

        int elem_count = Min((int)struct_type->aggregate.fields.count, (int)literal->elements.count);
        for (int i = 0; i < elem_count; i++) {
            Ast_Expr *elem = literal->elements[i];
            if (elem->invalid()) continue;
            
            Struct_Field_Info *field = &struct_type->aggregate.fields[i];
            if (!typecheck(field->type_info, elem->type_info)) {
                report_ast_error(elem, "cannot convert from '%s' to '%s'.\n", string_from_type(elem->type_info), string_from_type(field->type_info));
                literal->poison();
            }
        }
    } else if (specified_type->is_array_type()) {
        Ast_Type_Info *elem_type = specified_type->base;
        for (int i = 0; i < literal->elements.count; i++) {
            Ast_Expr *elem = literal->elements[i];
            if (elem->invalid()) break;

            if (!typecheck(elem_type, elem->type_info)) {
                report_ast_error(elem, "cannot convert from '%s' to '%s'.\n", string_from_type(elem->type_info), string_from_type(elem_type));
                literal->poison();
            }
        }
    }

    bool is_constant = true;
    for (int i = 0; i < literal->elements.count; i++) {
        Ast_Expr *elem = literal->elements[i];
        if (!elem->is_constant()) {
            is_constant = false;
        }
    }
    if (is_constant) {
        literal->expr_flags |= EXPR_FLAG_CONSTANT;
    }
}

bool inline is_constant_unary_op(Token_Kind op) {
    switch (op) {
    default:
        return false;
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_BANG:
        return true;
    }
}

void Resolver::resolve_unary_expr(Ast_Unary *unary) {
    resolve_expr(unary->elem);

    if (unary->elem->valid()) {
        if (unary->elem->type_info->is_custom_type()) {
            unary->expr_flags |= EXPR_FLAG_OP_CALL;
            resolve_user_defined_operator_expr(unary);
        } else if (unary->elem->valid() && !(unary->elem->type_info->type_flags & TYPE_FLAG_NUMERIC)) {
            report_ast_error(unary, "invalid operand '%s' of type '%s' in unary '%s'.\n", string_from_expr(unary->elem), string_from_type(unary->elem->type_info), string_from_token(unary->op.kind));
            unary->poison();
        }
        unary->type_info = unary->elem->type_info;
    } else {
        unary->poison();
    }

    if (unary->valid() && unary->elem->is_constant()) {
        if (is_constant_unary_op(unary->op.kind)) {
            unary->expr_flags |= EXPR_FLAG_CONSTANT;
            unary->eval = eval_unary_expr(unary);
        }
    }
}

void Resolver::resolve_literal(Ast_Literal *literal) {
    literal->expr_flags |= EXPR_FLAG_CONSTANT;

    switch (literal->literal_flags) {
    default:
        Assert(0);
        break;

    case LITERAL_NULL:
        literal->type_info = type_null;
        literal->eval.int_val = 0;
        break;

    case LITERAL_INT:
    case LITERAL_I32:
        literal->type_info = type_int;
        literal->eval.int_val = (s64)literal->int_val;
        break;

    case LITERAL_I8:
        literal->type_info = type_i8;
        literal->eval.int_val = (s64)literal->int_val;
        break;
    case LITERAL_I16:
        literal->type_info = type_i16;
        literal->eval.int_val = (s64)literal->int_val;
        break;
     case LITERAL_I64:
        literal->type_info = type_i64;
        literal->eval.int_val = (s64)literal->int_val;
        break;

     case LITERAL_U8:
        literal->type_info = type_u8;
        literal->eval.int_val = (s64)literal->int_val;
        break;
     case LITERAL_U16:
        literal->type_info = type_u16;
        literal->eval.int_val = (s64)literal->int_val;
        break;
     case LITERAL_U32:
        literal->type_info = type_u32;
        literal->eval.int_val = (s64)literal->int_val;
        break;
     case LITERAL_U64:
        literal->type_info = type_u64;
        literal->eval.int_val = (s64)literal->int_val;
        break;

    case LITERAL_BOOLEAN:
        literal->type_info = type_bool;
        literal->eval.int_val = literal->int_val;
        break;

    case LITERAL_FLOAT:
    case LITERAL_F32:
        literal->type_info = type_f32;
        literal->eval.float_val = literal->float_val;
        break;
    case LITERAL_F64:
        literal->type_info = type_f64;
        literal->eval.float_val = literal->float_val;
        break;

    case LITERAL_STRING:
        literal->type_info = ast_pointer_type_info(type_u8);
        break;
    }
}

void Resolver::resolve_ident(Ast_Ident *ident) {
    Ast_Decl *found = lookup(ident->name);

    if (found) {
        resolve_decl(found);
        ident->type_info = found->type_info;

        if (!(found->decl_flags & DECL_FLAG_TYPE)) {
            ident->expr_flags |= EXPR_FLAG_LVALUE;
        }

        ident->decl = found;

        if (found->invalid()) ident->poison();
    } else {
        report_undeclared(ident);
        ident->poison();
    }
}

void Resolver::resolve_call_expr(Ast_Call *call) {
    for (int i = 0; i < call->arguments.count; i++) {
        Ast_Expr *arg = call->arguments[i];
        resolve_expr(arg);
        if (arg->invalid()) call->poison();
    }

    Ast_Type_Info *elem_type = NULL;

    if (call->elem->kind == AST_IDENT) {
        Ast_Ident *name = static_cast<Ast_Ident*>(call->elem);
        bool overloaded = false;
        Ast_Decl *decl = lookup_overloaded(name->name, call->arguments, &overloaded);
        if (decl) {
            resolve_decl(decl);
            name->decl = decl;
            elem_type = decl->type_info;
        } else {
            if (overloaded) {
                report_ast_error(name, "no overloaded procedure matches all argument types.\n");
            } else {
                report_undeclared(name);
            }
            call->poison();
        }
    } else {
        resolve_expr(call->elem);
        elem_type = call->elem->type_info;
    }

    if (elem_type) {
        if (elem_type->is_proc_type()) {
            Ast_Proc *proc = static_cast<Ast_Proc*>(elem_type->decl);
            Ast_Proc_Type_Info *proc_type = static_cast<Ast_Proc_Type_Info*>(proc->type_info);
            call->type_info = proc_type->return_type;
            if (!proc->has_varargs && (call->arguments.count != proc_type->parameters.count)) {
                report_ast_error(call, "'%s' does not take %d arguments.\n", string_from_expr(call->elem), call->arguments.count);
                report_note(proc->start, call->file, "see declaration of '%s'.\n", proc->name->data);
                call->poison();
                return;
            }

            bool bad_arg = false;
            for (int i = 0, param_idx = 0; i < call->arguments.count; i++) {
                Ast_Param *param = proc->parameters[param_idx];
                Ast_Expr *arg = call->arguments[i];
                if (!param->is_vararg) {
                    if (arg->valid() && !typecheck(param->type_info, arg->type_info)) {
                        bad_arg = true;
                        report_ast_error(arg, "incompatible argument of type '%s' for parameter of type '%s'.\n", string_from_type(arg->type_info), string_from_type(param->type_info));
                        call->poison();
                    }
                    param_idx++;
                }
            }

            if (bad_arg) {
                report_note(proc->start, proc->file, "see declaration of '%s'.\n", proc->name->data);
            }
        } else {
            report_ast_error(call, "'%s' does not evaluate to a procedure.\n", string_from_expr(call->elem));
            call->poison();
        }
    }
}

void Resolver::resolve_deref_expr(Ast_Deref *deref) {
    resolve_expr(deref->elem);
    if (deref->elem->type_info->is_indirection_type()) {
        deref->type_info = deref->elem->type_info->deref();
    } else {
        report_ast_error(deref, "cannot dereference '%s', not a pointer type.\n", string_from_expr(deref->elem));
        deref->poison();
    }
}

void Resolver::resolve_access_expr(Ast_Access *access) {
    resolve_expr(access->parent);
    if (access->parent->invalid()) {
        access->poison();
        return;
    }

    if (access->parent->type_info->is_struct_access()) {
        access->expr_flags |= EXPR_FLAG_LVALUE;

        Ast_Type_Info *struct_type = access->parent->type_info;
        if (access->parent->type_info->is_pointer_type()) {
            struct_type = access->parent->type_info->base;
        }

        Struct_Field_Info *struct_field = struct_lookup(struct_type, access->name->name);
        if (!struct_field) {
            report_ast_error(access->parent, "'%s' is not a member of '%s'.\n", access->name->name->data, struct_type->decl->name->data);
            access->poison();
            return;
        }
        access->type_info = struct_field->type_info;
    } else if (access->parent->type_info->is_enum_type()) {
        if (access->parent->kind == AST_ACCESS) {
            Ast_Access *parent_access = static_cast<Ast_Access*>(access->parent);
            if (parent_access->parent->type_info->is_struct_type()) {
                report_ast_error(access, "cannot access expression of enum inside struct.\n");
                access->poison();
                return;
            }
        }

        Ast_Enum_Type_Info *enum_type = static_cast<Ast_Enum_Type_Info*>(access->parent->type_info);
        Ast_Enum_Field *enum_field = enum_lookup((Ast_Enum *)enum_type->decl, access->name->name);
        if (!enum_field) {
            report_ast_error(access->parent, "'%s' is not a member of '%s'.\n", access->name->name->data, enum_type->decl->name->data);
            access->poison();
            return;
        }
        access->type_info = access->parent->type_info;
        access->eval.int_val = enum_field->value;
        access->expr_flags |= EXPR_FLAG_CONSTANT;
    } else if (access->parent->type_info->is_array_type()) {
        access->expr_flags |= EXPR_FLAG_LVALUE;

        Ast_Type_Info *type = access->parent->type_info;
        Struct_Field_Info *struct_field = struct_lookup(type, access->name->name);
        if (!struct_field) {
            report_ast_error(access->parent, "'%s' is not a member of array type.\n", access->name->name->data);
            access->poison();
            return;
        }
        access->type_info = struct_field->type_info;
    } else if (access->parent->type_info->type_flags & TYPE_FLAG_STRING) {
        access->expr_flags |= EXPR_FLAG_LVALUE;

        Ast_Type_Info *type = access->parent->type_info;
        Struct_Field_Info *struct_field = struct_lookup(type, access->name->name);
        if (!struct_field) {
            report_ast_error(access->parent, "'%s' is not a member of string type.\n", access->name->name->data);
            access->poison();
            return;
        }
        access->type_info = struct_field->type_info;
    } else {
        report_ast_error(access->parent, "cannot access left of .'%s', illegal type '%s'.\n", access->name->name->data, string_from_type(access->parent->type_info));
        access->poison();
        return;
    }
}

void Resolver::resolve_range_expr(Ast_Range *range) {
    resolve_expr(range->lhs);
    resolve_expr(range->rhs);

    if (range->lhs->valid()) {
        if (!range->lhs->type_info->is_integral_type()) {
            report_ast_error(range->lhs, "'%s' is invalid range expression, not an integral type.\n", string_from_expr(range->lhs));
            range->poison();
        }
    }

    if (range->rhs->valid()) {
        if (!range->rhs->type_info->is_integral_type()) {
            report_ast_error(range->rhs, "'%s' is invalid range expression, not an integral type.\n", string_from_expr(range->rhs));
            range->poison();
        }
    }

    if (range->lhs->valid() && range->rhs->valid()) {
        if (typecheck(range->lhs->type_info, range->rhs->type_info)) {
            
        } else {
            report_ast_error(range, "mismatched types in range expression ('%s' and '%s').\n", string_from_type(range->lhs->type_info), string_from_type(range->rhs->type_info));
        }
    }

    if (range->lhs->is_constant() && range->rhs->is_constant()) range->expr_flags |= EXPR_FLAG_CONSTANT;

    range->type_info = range->lhs->type_info;
}

void Resolver::resolve_address_expr(Ast_Address *address) {
    resolve_expr(address->elem);

    if (address->elem->valid()) {
        address->type_info = ast_pointer_type_info(address->elem->type_info);

        if (!(address->elem->expr_flags & EXPR_FLAG_LVALUE)) {
            report_ast_error(address->elem, "cannot take address of '%s'.\n", string_from_expr(address->elem));
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

    case AST_NULL:
    {
        Ast_Null *null = static_cast<Ast_Null*>(expr);
        null->type_info = type_null;
        break;
    }

    case AST_PAREN:
    {
        Ast_Paren *paren = static_cast<Ast_Paren*>(expr);
        resolve_expr(paren->elem);
        paren->type_info = paren->elem->type_info;

        if (paren->elem->valid()) {
            if (paren->elem->is_constant()) {
                paren->expr_flags |= EXPR_FLAG_CONSTANT;
                paren->eval = paren->elem->eval;
            }
        } else {
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

    case AST_SUBSCRIPT:
    {
        Ast_Subscript *subscript = static_cast<Ast_Subscript *>(expr);
        resolve_subscript_expr(subscript);
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

    case AST_ASSIGNMENT:
    {
        Ast_Assignment *assignment = static_cast<Ast_Assignment*>(expr);
        resolve_assignment_expr(assignment);
        break;
    }

    case AST_ACCESS:
    {
        Ast_Access *access = static_cast<Ast_Access*>(expr);
        resolve_access_expr(access);
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
        param->type_info = type;
        parameters.push(type);
    }
    if (return_type == NULL) return_type = type_void;

    Ast_Proc_Type_Info *type = ast_proc_type_info(return_type, parameters);
    type->decl = proc;
    proc->type_info = type;

    if (proc->kind == AST_OPERATOR_PROC) {
        Ast_Operator_Proc *operator_proc = static_cast<Ast_Operator_Proc*>(proc);
        Ast_Proc_Type_Info *type = static_cast<Ast_Proc_Type_Info*>(operator_proc->type_info);

        if (type->parameters.count == 0) {
            report_ast_error(proc, "operator %s missing parameters.\n", string_from_token(operator_proc->op));
        }

        bool has_custom_type = false;
        for (int i = 0; i < type->parameters.count; i++) {
            Ast_Type_Info *param = type->parameters[i];
            if (!param->is_builtin_type()) {
                has_custom_type = true;
            }
        }

        if (!has_custom_type) {
            report_ast_error(proc, "operator %s must have at least one user-defined type.\n", string_from_token(operator_proc->op));
        }
    }
}

void Resolver::resolve_proc(Ast_Proc *proc) {
    resolve_proc_header(proc);
    if (in_global_scope() && proc->block) {
        // printf("RESOLVING '%s'\n", proc->name->data);
        Scope *scope = new_scope(SCOPE_PROC);
        proc->scope = scope;
        current_proc = proc;
        for (int i = 0; i < proc->parameters.count; i++) {
            Ast_Param *param = proc->parameters[i];
            add_entry(param);
            proc->local_vars.push(param);
        }
        resolve_block(proc->block);
        exit_scope();
    }
}

void Resolver::resolve_struct(Ast_Struct *struct_decl) {
    Auto_Array<Struct_Field_Info> struct_fields;
    for (int i = 0; i < struct_decl->fields.count; i++) {
        Ast_Struct_Field *field = struct_decl->fields[i];
        field->type_info = resolve_type(field->type_defn);

        Struct_Field_Info field_info = {};
        field_info.name = field->name;
        field_info.type_info = field->type_info;
        field_info.mem_offset = 0;
        struct_fields.push(field_info);
    }
    Ast_Type_Info *type_info = ast_struct_type_info(struct_fields);
    struct_decl->type_info = type_info;
    type_info->decl = struct_decl;
    type_info->name = struct_decl->name;
}

void Resolver::resolve_enum(Ast_Enum *enum_decl) {
    Auto_Array<Enum_Field_Info> enum_fields;
    for (int i = 0; i < enum_decl->fields.count; i++) {
        Ast_Enum_Field *field = enum_decl->fields[i];
        s64 value = (s64)i;
        field->value = value;

        for (int j = 0; j < enum_fields.count; j++) {
            Enum_Field_Info field_info = enum_fields[j];
            if (atoms_match(field->name, field_info.name)) {
                report_ast_error(field, "'%s' is already declared in enum.\n", field->name->data);
                break;
            }
        }

        Enum_Field_Info field_info = {};
        field_info.name = field->name;
        field_info.value = value;
        enum_fields.push(field_info);
    }

    Ast_Enum_Type_Info *type_info = ast_enum_type_info(enum_fields);
    enum_decl->type_info = type_info;
    type_info->decl = enum_decl;
    type_info->name = enum_decl->name;
    type_info->base = type_i32;
}

void Resolver::resolve_var(Ast_Var *var) {
    if (var->init && var->init->kind == AST_COMPOUND_LITERAL) {
        Ast_Compound_Literal *literal = static_cast<Ast_Compound_Literal*>(var->init);
        literal->visited = true;

        Auto_Array<Ast_Type_Info*> literal_types;
        for (int i = 0; i < literal->elements.count; i++) {
            Ast_Expr *element = literal->elements[i];
            resolve_expr(element);
            if (!literal_types.find(element->type_info)) {
                literal_types.push(element->type_info);
            }
        }

        Ast_Type_Info *literal_type_spec = resolve_type(literal->type_defn);
        Ast_Type_Info *specified_type = resolve_type(var->type_defn);

        if (specified_type && literal_type_spec) {
            if (typecheck(specified_type, literal_type_spec)) {
                var->type_info = specified_type;
                literal->type_info = specified_type;
            } else {
                report_ast_error(literal, "type mismatch, cannot convert from '%s' to '%s'.\n", string_from_type(literal_type_spec), string_from_type(specified_type));
                literal->poison();
                goto ERROR_BLOCK;
            }
        }

        Ast_Type_Info *type_info = specified_type;
        if (!type_info) {
            type_info = literal_type_spec;
        }

        var->type_info = type_info;

        if (type_info) {
            if (type_info->is_array_type()) {
                Ast_Type_Info *base_type = type_info->base;
                for (int i = 0; i < literal->elements.count; i++) {
                    Ast_Expr *element = literal->elements[i];
                    if (!typecheck(element->type_info, base_type)) {
                        report_ast_error(literal, "cannot convert from '%s' to '%s'.\n", string_from_type(element->type_info), string_from_type(base_type));
                        literal->poison();
                        goto ERROR_BLOCK;
                    }
                }
            } else if (type_info->is_struct_type()) {
                int count = (int)Min(type_info->aggregate.fields.count, literal->elements.count);
                for (int i = 0; i < count; i++) {
                    Ast_Expr *element = literal->elements[i];
                    Struct_Field_Info field = type_info->aggregate.fields[i];
                    if (!typecheck(field.type_info, element->type_info)) {
                        report_ast_error(literal, "cannot convert from '%s' to '%s'.\n", string_from_type(element->type_info), string_from_type(field.type_info));
                        literal->poison();
                        goto ERROR_BLOCK;
                    }
                }
            }
        } else {
            if (literal_types.count == 1) {
                Ast_Type_Info *type = ast_array_type_info(literal_types[0]);
                literal->type_info = type;
                var->type_info = type;
            } else {
                report_ast_error(literal, "cannot infer type of compound literal.\n");
                literal->poison();
                goto ERROR_BLOCK;
            }
        }

        if (var->type_info->is_array_type()) {
            Ast_Array_Type_Info *array_type = static_cast<Ast_Array_Type_Info*>(var->type_info);
            array_type->array_size = literal->elements.count;
        }
    } else {
        Ast_Type_Info *specified_type = NULL;
        if (var->type_defn) {
            specified_type = resolve_type(var->type_defn);
        }

        if (var->init) {
            resolve_expr(var->init);
            if (var->init->invalid()) {
                goto ERROR_BLOCK;
            }
        }

        if (var->type_defn && var->init) {
            var->type_info = specified_type;
            if (var->init->valid() && !typecheck(specified_type, var->init->type_info)) {
                report_ast_error(var->init, "cannot assign '%s' of type '%s' to '%s'\n", string_from_expr(var->init), string_from_type(var->init->type_info), var->name->data);
                goto ERROR_BLOCK;
            }
        } else if (var->type_defn != NULL) {
            var->type_info = specified_type;
        } else {
            var->type_info = var->init->type_info;
        }

        if (var->init && var->init->kind == AST_NULL) {
            var->init->type_info = specified_type;
        }
    }

    if (current_proc) {
        current_proc->local_vars.push(var);
    }

    return;

ERROR_BLOCK:
    var->poison();
}

void Resolver::resolve_param(Ast_Param *param) {
    if (!param->is_vararg) {
        Ast_Type_Info *type_info = resolve_type(param->type_defn);
        param->type_info = type_info;
    }
}

void Resolver::register_global_declarations() {
    Ast_Root *root = parser->root;

    global_scope = new_scope(SCOPE_GLOBAL);
    root->scope = global_scope;

    for (Builtin_Type_Kind builtin_type = BUILTIN_TYPE_VOID; builtin_type < BUILTIN_TYPE_COUNT; builtin_type = (Builtin_Type_Kind)(builtin_type + 1)) {
        Ast_Type_Info *type_info = g_builtin_types[builtin_type];
        if (type_info->name) {
            Ast_Type_Decl *decl = ast_type_decl(type_info->name, type_info);
            global_scope->declarations.push(decl);
        }
    }

    for (int i = 0; i < root->declarations.count; i++) {
        Ast_Decl *decl = root->declarations[i];
        Assert(decl->name);

        Ast_Decl *found = NULL;

        if (decl->kind == AST_PROC || decl->kind == AST_OPERATOR_PROC) {
        } else {
            found = lookup(decl->name);
        }

        if (!found) {
            add_entry(decl);
        } else {
            report_redeclaration(decl);
        }

    }
}

void Resolver::resolve_decl(Ast_Decl *decl) {
    //@Fix This is to prevent a bug where the procedure body doesn't get resolved for some reason
    if (in_global_scope() && decl->kind == AST_PROC) {
        decl->resolve_state = RESOLVE_UNSTARTED;
    }

    if (decl->resolve_state == RESOLVE_DONE) {
        return;
    } else if (decl->resolve_state == RESOLVE_STARTED &&
        decl != current_proc) {
        report_ast_error(decl, "Illegal recursive declaration.\n");
        decl->poison();
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
    case AST_OPERATOR_PROC:
    case AST_PROC:
    {
        Ast_Proc *proc = static_cast<Ast_Proc*>(decl);
        resolve_proc(proc);
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

}

void Resolver::resolve_overloaded_proc(Ast_Proc *proc) {
    Ast_Decl *found = NULL;
    Scope *scope = global_scope;
    for (int i = 0; i < scope->declarations.count; i++) {
        Ast_Decl *decl = scope->declarations[i];

        // @Note Do not go further down for declarations later defined that could be conflicts.
        if (proc == decl) break; 
        
        if (!atoms_match(proc->name, decl->name)) {
            continue;
        }

        if (decl->kind != AST_PROC && decl->kind != AST_OPERATOR_PROC) {
            found = decl;
            break;
        }

        Ast_Proc *other = static_cast<Ast_Proc*>(decl);

        b32 overloaded = true;
        Ast_Proc_Type_Info *proc_type = static_cast<Ast_Proc_Type_Info*>(proc->type_info);
        Ast_Proc_Type_Info *other_type = static_cast<Ast_Proc_Type_Info*>(other->type_info);
        if (proc_type->parameters.count == other_type->parameters.count) {
            b32 mismatch = false;
            for (int p = 0; p < proc_type->parameters.count; p++) {
                Ast_Type_Info *p0 = proc_type->parameters[p];
                Ast_Type_Info *p1 = other_type->parameters[p];

                if (!typecheck(p0, p1)) {
                    mismatch = true;
                    break;
                }
            }

            if (!mismatch) {
                overloaded = false;
            }
        }

        if (!overloaded) {
            found = proc;
            break;
        }
    }

    if (found) {
        report_redeclaration(proc);
    }
}

void Resolver::resolve() {
    register_global_declarations();

    Ast_Root *root = parser->root;
    for (int i = 0; i < root->declarations.count; i++) {
        Ast_Decl *decl = root->declarations[i];
        resolve_decl(decl);
    }

    for (int i = 0; i < root->declarations.count; i++) {
        Ast_Decl *decl = root->declarations[i];
        if (decl->kind == AST_PROC || decl->kind == AST_OPERATOR_PROC) {
            Ast_Proc *proc = static_cast<Ast_Proc*>(decl);
            resolve_overloaded_proc(proc);
        }
    }
}
