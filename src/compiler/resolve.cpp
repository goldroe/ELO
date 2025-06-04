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

Ast_Operator_Proc *Resolver::lookup_user_defined_binary_operator(OP op, Type *lhs, Type *rhs) {
    Ast_Operator_Proc *result = NULL;
    Scope *scope = global_scope;
    for (int i = 0; i < scope->declarations.count; i++) {
        Ast_Decl *decl = scope->declarations[i];
        if (decl->kind == AST_OPERATOR_PROC) {
            Ast_Operator_Proc *proc = static_cast<Ast_Operator_Proc*>(decl);
            resolve_proc_header(proc);
            Proc_Type *proc_type = static_cast<Proc_Type*>(proc->type);
            if (proc->op == op && typecheck(proc_type->parameters[0], lhs) && typecheck(proc_type->parameters[1], rhs)) {
                result = proc;
                break;
            }
        }
    }
    return result;
}

Ast_Operator_Proc *Resolver::lookup_user_defined_unary_operator(OP op, Type *type) {
    Ast_Operator_Proc *result = NULL;
    Scope *scope = global_scope;
    for (int i = 0; i < scope->declarations.count; i++) {
        Ast_Decl *decl = scope->declarations[i];
        if (decl->kind == AST_OPERATOR_PROC) {
            Ast_Operator_Proc *proc = static_cast<Ast_Operator_Proc*>(decl);
            resolve_proc_header(proc);
            Proc_Type *proc_type = static_cast<Proc_Type*>(proc->type);
            if (proc->op == op && typecheck(proc_type->parameters[0], type)) {
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
            Proc_Type *proc_type = static_cast<Proc_Type*>(proc->type);
            if (proc_type->parameters.count == arguments.count) {
                bool match = true;
                for (int j = 0; j < arguments.count; j++) {
                    Type *param = proc_type->parameters[j];
                    Ast_Expr *arg = arguments[j];
                    if (!typecheck(param, arg->type)) {
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

internal Struct_Field_Info *struct_lookup(Type *type, Atom *name) {
    for (int i = 0; i < type->aggregate.fields.count; i++) {
        Struct_Field_Info *field = &type->aggregate.fields[i];
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
        cond->type->is_struct_type()) {
        report_ast_error(cond, "'%s' is not a valid conditional expression.\n", string_from_expr(cond));
    }

    resolve_block(if_stmt->block);
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


void Resolver::resolve_return_stmt(Ast_Return *return_stmt) {
    Proc_Type *proc_type = static_cast<Proc_Type*>(current_proc->type);
    current_proc->returns = true;

    if (return_stmt->expr) {
        resolve_expr(return_stmt->expr);

        if (proc_type->return_type == type_void) {
            report_ast_error(return_stmt->expr, "returning value on a void procedure.\n");
            return_stmt->poison();
        }

        if (return_stmt->expr->valid() &&
            !typecheck(proc_type->return_type, return_stmt->expr->type)) {
            report_ast_error(return_stmt->expr, "invalid return type '%s', procedure returns '%s'.\n", string_from_type(return_stmt->expr->type), string_from_type(proc_type->return_type)); 
            return_stmt->poison();
        }
    } else {
        if (proc_type->return_type != type_void) {
            report_ast_error(return_stmt, "procedure expects a return type '%s'.\n", string_from_type(proc_type->return_type));
            return_stmt->poison();
        }
    }
}

void Resolver::resolve_fallthrough_stmt(Ast_Fallthrough *fallthrough) {
    if (!fallthrough->target) {
        report_parser_error(fallthrough, "illegal fallthrough, must be placed at end of a case block.\n");
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

    if (cond->valid() && !cond->type->is_conditional_type()) {
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
        if (!ifcase->cond->type->is_integral_type()) {
            ifcase->switch_jumptable = false;
        }
    } else {
        ifcase->switch_jumptable = false;
    }

    breakcont_stack.push(ifcase);

    for (Ast_Case_Label *case_label : ifcase->cases) {
        resolve_expr(case_label->cond);

        if (case_label->cond) {
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
                if (!typecheck(ifcase->cond->type, case_label->cond->type)) {
                    report_ast_error(case_label->cond, "'%s' is illegal type for case expression.\n", string_from_type(case_label->cond->type));
                    ifcase->poison();
                }
            }
        } else {
            if (case_label->cond && case_label->cond->kind == AST_RANGE) {
                //@Todo report crashes
                report_ast_error(case_label, "illegal range condition in an ifcase without an initial condition to compare.\n");
                ifcase->poison();
            }
        }

        resolve_block(case_label->block);
    }

    breakcont_stack.pop();

    if (ifcase->switch_jumptable) {
        std::unordered_set<u64> switch_constants;

        for (Ast_Case_Label *label : ifcase->cases) {
            if (label->cond && label->cond->kind == AST_RANGE) {
                Ast_Range *range = static_cast<Ast_Range*>(label->cond);
                u64 min = range->lhs->eval.int_val;
                u64 max = range->rhs->eval.int_val;
                for (u64 c = min; c <= max; c++) {
                    if (switch_constants.find(c) != switch_constants.end()) {
                        ifcase->poison();
                        report_ast_error(label, "case value '%llu' in range '%s' already used.\n", c, string_from_expr(label->cond));
                        break;
                    } else {
                        switch_constants.insert(c);
                    }
                }
            } else if (label->cond) {
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
        for (Ast_If *node = if_stmt; node; node = (Ast_If *)node->next) {
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
    case AST_FALLTHROUGH:
    {
        Ast_Fallthrough *fallthrough = static_cast<Ast_Fallthrough*>(stmt);
        resolve_fallthrough_stmt(fallthrough);
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

Type *Resolver::resolve_type(Ast_Type_Defn *type_defn) {
    Type *type = NULL;
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
                if (type) type->base = decl->type;
                else type = decl->type;
            } else {
                report_ast_error(t, "undeclared type '%s'.\n", t->name->data);
                return type_poison;
            }
            break;
        }

        case TYPE_DEFN_POINTER:
        {
            Type *ptr = pointer_type(type);
            type = ptr;
            break;
        }

        case TYPE_DEFN_ARRAY:
        {
            Ast_Expr *array_size = t->array_size;
            Array_Type *array = array_type(type);

            if (array_size) {
                resolve_expr(array_size);
                if (array_size->type->is_integral_type()) {
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

        case TYPE_DEFN_PROC:
        {
            Proc_Type *proc_ty = AST_NEW(Proc_Type);
            proc_ty->id = TYPEID_PROC;
            for (Ast_Type_Defn *param : t->proc.parameters) {
                Type *param_ty = resolve_type(param);
                proc_ty->parameters.push(param_ty);
            }
            if (t->proc.return_type) {
                proc_ty->return_type = resolve_type(t->proc.return_type);
            } else {
                proc_ty->return_type = type_void;
            }
            type = proc_ty;
            break;
        }
        }
    }
    return type;
}

Eval Resolver::eval_unary_expr(Ast_Unary *u) {
    Eval result = {};
    Eval e = u->elem->eval;
    if (u->elem->type->is_integral_type()) {
        switch (u->op) {
        case OP_UNARY_PLUS:
            result.int_val = e.int_val;
            break;
        case OP_UNARY_MINUS:
            result.int_val = -e.int_val;
            break;
        case OP_NOT:
            result.int_val = !e.int_val;
            break;
        case OP_BIT_NOT:
            result.int_val = ~e.int_val;
            break;
        }
    } else if (u->elem->type->is_float_type()) {
        switch (u->op) {
        case OP_UNARY_PLUS:
            result.float_val = e.float_val;
            break;
        case OP_UNARY_MINUS:
            result.float_val = -e.float_val;
            break;
        case OP_NOT:
            result.float_val = !e.float_val;
            break;
        case OP_BIT_NOT:
            Assert(0);
            break;
        }
    }
    return result;
}

Eval Resolver::eval_binary_expr(Ast_Binary *b) {
    Eval result = {};
    Eval lhs = b->lhs->eval;
    Eval rhs = b->rhs->eval;    

    if (b->lhs->type->is_integral_type()) {
        switch (b->op) {
        case OP_ADD:
            result.int_val = lhs.int_val + rhs.int_val;
            break;
        case OP_SUB:
            result.int_val = lhs.int_val - rhs.int_val;
            break;
        case OP_MUL:
            result.int_val = lhs.int_val * rhs.int_val;
            break;
        case OP_DIV:
            result.int_val = lhs.int_val / rhs.int_val;
            break;
        case OP_MOD:
            result.int_val = lhs.int_val % rhs.int_val;
            break;
        case OP_NEQ:
            result.int_val = lhs.int_val != rhs.int_val;
            break;
        case OP_LT:
            result.int_val = lhs.int_val < rhs.int_val;
            break;
        case OP_LTEQ:
            result.int_val = lhs.int_val <= rhs.int_val;
            break;
        case OP_GT:
            result.int_val = lhs.int_val > rhs.int_val;
            break;
        case OP_GTEQ:
            result.int_val = lhs.int_val >= rhs.int_val;
            break;
        case OP_BIT_AND:
            result.int_val = lhs.int_val & rhs.int_val;
            break;
        case OP_AND:
            result.int_val = lhs.int_val && rhs.int_val;
            break;
        case OP_BIT_OR:
            result.int_val = lhs.int_val | rhs.int_val;
            break;
        case OP_OR:
            result.int_val = lhs.int_val || rhs.int_val;
            break;
        case OP_XOR:
            result.int_val = lhs.int_val ^ rhs.int_val;
            break;
        }
    } else if (b->lhs->type->is_float_type()) {
        switch (b->op) {
        case OP_ADD:
            result.float_val = lhs.float_val + rhs.float_val;
            break;
        case OP_SUB:
            result.float_val = lhs.float_val - rhs.float_val;
            break;
        case OP_MUL:
            result.float_val = lhs.float_val * rhs.float_val;
            break;
        case OP_DIV:
            result.float_val = lhs.float_val / rhs.float_val;
            break;
        case OP_MOD:
            result.float_val = fmod(lhs.float_val, rhs.float_val);
            break;
        case OP_NEQ:
            result.float_val = lhs.float_val != rhs.float_val;
            break;
        case OP_LT:
            result.float_val = lhs.float_val < rhs.float_val;
            break;
        case OP_LTEQ:
            result.float_val = lhs.float_val <= rhs.float_val;
            break;
        case OP_GT:
            result.float_val = lhs.float_val > rhs.float_val;
            break;
        case OP_GTEQ:
            result.float_val = lhs.float_val >= rhs.float_val;
            break;
        case OP_AND:
            result.float_val = lhs.float_val && rhs.float_val;
            break;
        case OP_OR:
            result.float_val = lhs.float_val || rhs.float_val;
            break;
        }        
    }

    return result;
}

void Resolver::resolve_assignment_expr(Ast_Assignment *assignment) {
    Ast_Expr *lhs = assignment->lhs;
    Ast_Expr *rhs = assignment->rhs;

    resolve_expr(lhs);
    resolve_expr(rhs);

    //@Note Give null expr type of lhs
    if (rhs->kind == AST_NULL) {
        rhs->type = lhs->type;
    }

    if (lhs->valid() && !(lhs->expr_flags & EXPR_FLAG_LVALUE)) {
        report_ast_error(lhs, "cannot assign to '%s', is not an l-value.\n", string_from_expr(lhs));
        assignment->poison();
    }

    if (lhs->valid() && rhs->valid()) {
        if (!typecheck(lhs->type, rhs->type)) {
            report_ast_error(rhs, "type mismatch, cannot assign to '%s' from '%s'.\n", string_from_type(lhs->type), string_from_type(rhs->type));
            assignment->poison();
        }
    }
    assignment->type = lhs->type;
}

void Resolver::resolve_builtin_unary_expr(Ast_Unary *expr) {
    Ast_Expr *elem = expr->elem;
    switch (expr->op) {
    case OP_UNARY_PLUS:
        if (elem->type->is_numeric_type()) {
            expr->type = elem->type;
        } else {
            report_ast_error(elem, "invalid operand '%s' of type '%s' in unary '%s'.\n", string_from_expr(elem), string_from_type(elem->type), string_from_operator(expr->op));
            expr->poison();
        }
    case OP_UNARY_MINUS:
        if (elem->type->is_numeric_type() && !elem->type->is_pointer_type()) {
            expr->type = elem->type;
        } else {
            report_ast_error(expr, "invalid operand '%s' of type '%s' in unary '%s'.\n", string_from_expr(elem), string_from_type(elem->type), string_from_operator(expr->op));
            expr->poison();
        }
    case OP_NOT:
    case OP_BIT_NOT:
        if (elem->type->is_numeric_type()) {
            expr->type = elem->type;
        } else {
            report_ast_error(expr, "invalid operand '%s' of type '%s' in unary '%s'.\n", string_from_expr(elem), string_from_type(elem->type), string_from_operator(expr->op));
            expr->poison();
        }
        break;
    }
}

void Resolver::resolve_unary_expr(Ast_Unary *expr) {
    resolve_expr(expr->elem);
    if (expr->elem->valid()) {
        if (expr->elem->type->is_user_defined_type()) {
            Ast_Operator_Proc *proc = lookup_user_defined_unary_operator(expr->op, expr->elem->type);
            if (proc) {
                resolve_proc_header(proc);
                expr->expr_flags |= EXPR_FLAG_OP_CALL;
                expr->proc = proc;
                if (proc->valid()) {
                    expr->type = static_cast<Proc_Type*>(proc->type)->return_type;
                }
            } else {
                report_ast_error(expr, "could not find operator'%s' (%s).\n", string_from_operator(expr->op), string_from_type(expr->elem->type));
            }
        } else {
            resolve_builtin_unary_expr(expr);
            if (expr->valid() && expr->elem->is_constant()) {
                expr->expr_flags |= EXPR_FLAG_CONSTANT;
                expr->eval = eval_unary_expr(expr);
            }
        }
    } else {
        expr->poison();
    }
}

void Resolver::resolve_builtin_binary_expr(Ast_Binary *expr) {
    Ast_Expr *lhs = expr->lhs;
    Ast_Expr *rhs = expr->rhs;
    Assert(lhs->valid() && rhs->valid());

    switch (expr->op) {
    case OP_ADD:
        if (lhs->type->is_pointer_type() && rhs->type->is_pointer_type()) {
            report_ast_error(expr, "cannot add two pointers.\n");
        } else if (lhs->type->is_pointer_type()) {
            if (!rhs->type->is_integral_type()) {
                report_ast_error(rhs, "pointer addition requires integral operand.\n");
                expr->poison();
            }
            expr->type = lhs->type;
        } else if (rhs->type->is_pointer_type()) {
            if (!lhs->type->is_integral_type()) {
                report_ast_error(lhs, "pointer addition requires integral operand.\n");
                expr->poison();
            }
            expr->type = rhs->type;
        } else {
            if (!lhs->type->is_numeric_type()) {
                report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(lhs), string_from_operator(expr->op));
                expr->poison();
            }
            if (!rhs->type->is_numeric_type()) {
                report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(rhs), string_from_operator(expr->op));
                expr->poison();
            }
            expr->type = lhs->type;
        }
        break;

    case OP_SUB:
        if (lhs->type->is_pointer_type() && rhs->type->is_pointer_type()) {
            if (lhs->type != rhs->type) {
                report_ast_error(expr, "'%s' and '%s' are incompatible pointer types.\n", string_from_type(lhs->type), string_from_type(rhs->type));
                expr->poison();
            }
            expr->type = lhs->type;
        } else if (lhs->type->is_pointer_type()) {
            if (!rhs->type->is_integral_type()) {
                report_ast_error(rhs, "pointer subtraction requires pointer or integral operand.\n");
                expr->poison();
            }
        } else if (rhs->type->is_pointer_type()) {
            report_ast_error(lhs, "pointer can only be subtracted from another pointer.\n");
            expr->poison();
        } else {
            if (!lhs->type->is_numeric_type()) {
                report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(lhs), string_from_operator(expr->op));
                expr->poison();
            }
            if (!rhs->type->is_numeric_type()) {
                report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(rhs), string_from_operator(expr->op));
                expr->poison();
            }
            expr->type = lhs->type;
        }
        break;

    case OP_MUL:
    case OP_DIV:
    case OP_MOD:
        expr->type = lhs->type;
        if (!lhs->type->is_numeric_type() || lhs->type->is_pointer_type()) {
            report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(lhs), string_from_operator(expr->op));
            expr->poison();
        }
        if (!rhs->type->is_numeric_type() || rhs->type->is_pointer_type()) {
            report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(rhs), string_from_operator(expr->op));
            expr->poison();
        }
        break;

    case OP_BIT_AND:
    case OP_BIT_OR:
    case OP_XOR:
    case OP_LSH:
    case OP_RSH:
        expr->type = lhs->type;
        if (!lhs->type->is_integral_type()) {
            report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(lhs), string_from_operator(expr->op));
            expr->poison();
        }
        if (!rhs->type->is_integral_type()) {
            report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(rhs), string_from_operator(expr->op));
            expr->poison();
        }
        break;

    case OP_EQ:
    case OP_NEQ:
    case OP_LT:
    case OP_LTEQ:
    case OP_GT:
    case OP_GTEQ:
        expr->type = type_bool;
        if (lhs->type->is_struct_type()) {
            report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(lhs), string_from_operator(expr->op));
            expr->poison();
        }
        if (rhs->type->is_struct_type()) {
            report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(rhs), string_from_operator(expr->op));
            expr->poison();
        }
        break;

    case OP_OR:
    case OP_AND:
        expr->type = type_bool;
        //@Note Give null expr type of lhs
        if (rhs->kind == AST_NULL) {
            rhs->type = lhs->type;
        }
        if (!lhs->type->is_numeric_type()) {
            report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(lhs), string_from_operator(expr->op));
            expr->poison();
        }
        if (!rhs->type->is_numeric_type()) {
            report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", string_from_expr(rhs), string_from_operator(expr->op));
            expr->poison();
        }
        break;
    }

    if (expr->valid() &&
        lhs->is_constant() && rhs->is_constant()) {
        expr->expr_flags |= EXPR_FLAG_CONSTANT;
        expr->eval = eval_binary_expr(expr);
    }
}

void Resolver::resolve_binary_expr(Ast_Binary *expr) {
    Ast_Expr *lhs = expr->lhs;
    Ast_Expr *rhs = expr->rhs;

    resolve_expr(lhs);
    resolve_expr(rhs);

    if (lhs->invalid()) {
        expr->poison();
    }
    if (rhs->invalid()) {
        expr->poison();
    }

    if (lhs->valid() && rhs->valid()) {
        if (lhs->type->is_user_defined_type() || rhs->type->is_user_defined_type()) {
            Ast_Operator_Proc *proc = lookup_user_defined_binary_operator(expr->op, expr->lhs->type, expr->rhs->type);
            if (proc) {
                resolve_proc_header(proc);
                expr->proc = proc;
                expr->expr_flags |= EXPR_FLAG_OP_CALL;
                if (proc->valid()) {
                    expr->type = static_cast<Proc_Type*>(proc->type)->return_type;
                }
            } else {
                report_ast_error(expr, "could not find operator'%s' (%s,%s).\n", string_from_operator(expr->op), string_from_type(expr->lhs->type), string_from_type(expr->rhs->type));
                expr->poison();
            }
        } else {
            resolve_builtin_binary_expr(expr);  
        }
    }
}

void Resolver::resolve_cast_expr(Ast_Cast *cast) {
    resolve_expr(cast->elem);
    cast->type = resolve_type(cast->type_defn);

    if (cast->elem->valid()) {
        if (!typecheck_castable(cast->type, cast->elem->type)) {
            report_ast_error(cast->elem, "cannot cast '%s' as '%s' from '%s'.\n", string_from_expr(cast->elem), string_from_type(cast->type), string_from_type(cast->elem->type));
        }
    } else {
        cast->poison();
    }
}

void Resolver::resolve_subscript_expr(Ast_Subscript *subscript) {
    resolve_expr(subscript->expr);
    resolve_expr(subscript->index);

    if (subscript->expr->valid()) {
        if (subscript->expr->type->is_indirection_type()) {
            subscript->type = subscript->expr->type->base;
        } else {
            report_ast_error(subscript->expr, "'%s' is not a pointer or array type.\n", string_from_expr(subscript->expr));
            subscript->poison();
        }
    } else {
        subscript->poison();
    }

    if (subscript->index->valid()) {
        if (!subscript->index->type->is_integral_type()) {
            report_ast_error(subscript->index, "array subscript is not of integral type.\n");
            subscript->poison();
        }
    } else {
        subscript->poison();
    }
}

void Resolver::resolve_compound_literal(Ast_Compound_Literal *literal) {
    Type *specified_type = resolve_type(literal->type_defn);

    literal->type = specified_type;

    for (int i = 0; i < literal->elements.count; i++) {
        Ast_Expr *elem = literal->elements[i];
        resolve_expr(elem);

    }

    if (specified_type->is_struct_type()) {
        Type *struct_type = specified_type;
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
            if (!typecheck(field->type, elem->type)) {
                report_ast_error(elem, "cannot convert from '%s' to '%s'.\n", string_from_type(elem->type), string_from_type(field->type));
                literal->poison();
            }
        }
    } else if (specified_type->is_array_type()) {
        Type *elem_type = specified_type->base;
        for (int i = 0; i < literal->elements.count; i++) {
            Ast_Expr *elem = literal->elements[i];
            if (elem->invalid()) break;

            if (!typecheck(elem_type, elem->type)) {
                report_ast_error(elem, "cannot convert from '%s' to '%s'.\n", string_from_type(elem->type), string_from_type(elem_type));
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

void Resolver::resolve_literal(Ast_Literal *literal) {
    literal->expr_flags |= EXPR_FLAG_CONSTANT;

    switch (literal->literal_flags) {
    default:
        Assert(0);
        break;

    case LITERAL_NULL:
        literal->type = type_null;
        literal->eval.int_val = 0;
        break;

    case LITERAL_INT:
    case LITERAL_I32:
        literal->type = type_int;
        literal->eval.int_val = (s64)literal->int_val;
        break;

    case LITERAL_I8:
        literal->type = type_i8;
        literal->eval.int_val = (s64)literal->int_val;
        break;
    case LITERAL_I16:
        literal->type = type_i16;
        literal->eval.int_val = (s64)literal->int_val;
        break;
     case LITERAL_I64:
        literal->type = type_i64;
        literal->eval.int_val = (s64)literal->int_val;
        break;

     case LITERAL_U8:
        literal->type = type_u8;
        literal->eval.int_val = (s64)literal->int_val;
        break;
     case LITERAL_U16:
        literal->type = type_u16;
        literal->eval.int_val = (s64)literal->int_val;
        break;
     case LITERAL_U32:
        literal->type = type_u32;
        literal->eval.int_val = (s64)literal->int_val;
        break;
     case LITERAL_U64:
        literal->type = type_u64;
        literal->eval.int_val = (s64)literal->int_val;
        break;

    case LITERAL_BOOLEAN:
        literal->type = type_bool;
        literal->eval.int_val = literal->int_val;
        break;

    case LITERAL_FLOAT:
    case LITERAL_F32:
        literal->type = type_f32;
        literal->eval.float_val = literal->float_val;
        break;
    case LITERAL_F64:
        literal->type = type_f64;
        literal->eval.float_val = literal->float_val;
        break;

    case LITERAL_STRING:
        literal->type = pointer_type(type_u8);
        break;
    }
}

void Resolver::resolve_ident(Ast_Ident *ident) {
    Ast_Decl *found = lookup(ident->name);

    if (found) {
        resolve_decl(found);
        ident->type = found->type;

        if (!(found->decl_flags & DECL_FLAG_TYPE)) {
            ident->expr_flags |= EXPR_FLAG_LVALUE;
        }

        ident->ref = found;

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

    Ast_Expr *elem = call->elem;

    Ast_Proc *proc = nullptr;

    if (call->elem->kind == AST_IDENT) {
        Ast_Proc *proc = nullptr;
        Ast_Ident *name = static_cast<Ast_Ident*>(call->elem);
        bool overloaded = false;
        Ast_Decl *decl = lookup_overloaded(name->name, call->arguments, &overloaded);
        if (decl) {
            resolve_decl(decl);
            name->ref = decl;
            elem->type = decl->type;
            proc = static_cast<Ast_Proc*>(decl);
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
    }

    Proc_Type *proc_type = nullptr;

    if (elem->type->is_proc_type()) {
        proc_type = static_cast<Proc_Type*>(elem->type);
    } else {
        report_ast_error(call, "'%s' does not evaluate to a procedure.\n", string_from_expr(call->elem));
        call->poison();
        return;
    }

    if ((proc_type->has_varargs && (call->arguments.count < proc_type->parameters.count - 1))
        || (!proc_type->has_varargs && (call->arguments.count != proc_type->parameters.count))) {
        report_ast_error(call, "'%s' does not take %d arguments.\n", string_from_expr(elem), call->arguments.count);
        if (proc) {
            report_note(proc->start, call->file, "see declaration of '%s'.\n", proc->name->data);
        }
        call->poison();
        return;
    }

    bool bad_arg = false;
    for (int i = 0, param_idx = 0; i < call->arguments.count; i++) {
        Type *param_type = proc_type->parameters[param_idx];
        Ast_Expr *arg = call->arguments[i];
        if (param_type == NULL) { //@Note @Fix this is a vararg type
            if (arg->valid() && !typecheck(param_type, arg->type)) {
                bad_arg = true;
                report_ast_error(arg, "incompatible argument of type '%s' for parameter of type '%s'.\n", string_from_type(arg->type), string_from_type(param_type));
                call->poison();
            }
            param_idx++;
        }
    }
    if (proc && bad_arg) {
        report_note(proc->start, proc->file, "see declaration of '%s'.\n", proc->name->data);
        call->poison();
    }

    call->type = proc_type->return_type;
}

void Resolver::resolve_deref_expr(Ast_Deref *deref) {
    resolve_expr(deref->elem);
    if (deref->elem->type->is_indirection_type()) {
        deref->type = deref->elem->type->base;
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

    if (access->parent->type->is_struct_or_pointer()) {
        access->expr_flags |= EXPR_FLAG_LVALUE;

        Type *struct_type = access->parent->type;
        if (access->parent->type->is_pointer_type()) {
            struct_type = access->parent->type->base;
        }

        Struct_Field_Info *struct_field = struct_lookup(struct_type, access->name->name);
        if (!struct_field) {
            report_ast_error(access->parent, "'%s' is not a member of '%s'.\n", access->name->name->data, struct_type->decl->name->data);
            access->poison();
            return;
        }
        access->type = struct_field->type;
    } else if (access->parent->type->is_enum_type()) {
        if (access->parent->kind == AST_ACCESS) {
            Ast_Access *parent_access = static_cast<Ast_Access*>(access->parent);
            if (parent_access->parent->type->is_struct_type()) {
                report_ast_error(access, "cannot access expression of enum inside struct.\n");
                access->poison();
                return;
            }
        }

        Enum_Type *enum_type = static_cast<Enum_Type*>(access->parent->type);
        Ast_Enum_Field *enum_field = enum_lookup((Ast_Enum *)enum_type->decl, access->name->name);
        if (!enum_field) {
            report_ast_error(access->parent, "'%s' is not a member of '%s'.\n", access->name->name->data, enum_type->decl->name->data);
            access->poison();
            return;
        }
        access->type = access->parent->type;
        access->eval.int_val = enum_field->value;
        access->expr_flags |= EXPR_FLAG_CONSTANT;
    } else if (access->parent->type->is_array_type()) {
        access->expr_flags |= EXPR_FLAG_LVALUE;

        Type *type = access->parent->type;
        Struct_Field_Info *struct_field = struct_lookup(type, access->name->name);
        if (!struct_field) {
            report_ast_error(access->parent, "'%s' is not a member of array type.\n", access->name->name->data);
            access->poison();
            return;
        }
        access->type = struct_field->type;
    } else if (access->parent->type->id == TYPEID_STRING) {
        access->expr_flags |= EXPR_FLAG_LVALUE;

        Type *type = access->parent->type;
        Struct_Field_Info *struct_field = struct_lookup(type, access->name->name);
        if (!struct_field) {
            report_ast_error(access->parent, "'%s' is not a member of string type.\n", access->name->name->data);
            access->poison();
            return;
        }
        access->type = struct_field->type;
    } else {
        report_ast_error(access->parent, "cannot access left of .'%s', illegal type '%s'.\n", access->name->name->data, string_from_type(access->parent->type));
        access->poison();
        return;
    }
}

void Resolver::resolve_range_expr(Ast_Range *range) {
    resolve_expr(range->lhs);
    resolve_expr(range->rhs);

    if (range->lhs->valid()) {
        if (!range->lhs->type->is_integral_type()) {
            report_ast_error(range->lhs, "'%s' is invalid range expression, not an integral type.\n", string_from_expr(range->lhs));
            range->poison();
        }
    }

    if (range->rhs->valid()) {
        if (!range->rhs->type->is_integral_type()) {
            report_ast_error(range->rhs, "'%s' is invalid range expression, not an integral type.\n", string_from_expr(range->rhs));
            range->poison();
        }
    }

    if (range->lhs->valid() && range->rhs->valid()) {
        if (typecheck(range->lhs->type, range->rhs->type)) {
            
        } else {
            report_ast_error(range, "mismatched types in range expression ('%s' and '%s').\n", string_from_type(range->lhs->type), string_from_type(range->rhs->type));
        }
    }

    if (range->lhs->is_constant() && range->rhs->is_constant()) range->expr_flags |= EXPR_FLAG_CONSTANT;

    range->type = range->lhs->type;
}

void Resolver::resolve_address_expr(Ast_Address *address) {
    resolve_expr(address->elem);

    if (address->elem->valid()) {
        address->type = pointer_type(address->elem->type);

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
        null->type = type_null;
        break;
    }

    case AST_PAREN:
    {
        Ast_Paren *paren = static_cast<Ast_Paren*>(expr);
        resolve_expr(paren->elem);
        paren->type = paren->elem->type;

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
    }

    expr->visited = true;
}

void Resolver::resolve_proc_header(Ast_Proc *proc) {
    Type *return_type = resolve_type(proc->return_type_defn);
    Auto_Array<Type*> parameters;
    bool has_varargs = false;
    for (int i = 0; i < proc->parameters.count; i++) {
        Ast_Param *param = proc->parameters[i];
        if (param->is_vararg) has_varargs = true;
        Type *type = resolve_type(param->type_defn);
        param->type = type;
        parameters.push(type);
    }
    if (return_type == NULL) return_type = type_void;

    Proc_Type *type = proc_type(return_type, parameters);
    type->has_varargs = has_varargs;
    type->decl = proc;
    proc->type = type;

    if (proc->kind == AST_OPERATOR_PROC) {
        Ast_Operator_Proc *operator_proc = static_cast<Ast_Operator_Proc*>(proc);
        Proc_Type *type = static_cast<Proc_Type*>(operator_proc->type);

        if (type->parameters.count == 0) {
            report_ast_error(proc, "operator %s missing parameters.\n", string_from_operator(operator_proc->op));
        }

        bool has_custom_type = false;
        for (int i = 0; i < type->parameters.count; i++) {
            Type *param = type->parameters[i];
            if (param->is_user_defined_type()) {
                has_custom_type = true;
            }
        }

        if (!has_custom_type) {
            report_ast_error(proc, "operator %s must have at least one user-defined type.\n", string_from_operator(operator_proc->op));
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
        field->type = resolve_type(field->type_defn);

        Struct_Field_Info field_info = {};
        field_info.name = field->name;
        field_info.type = field->type;
        field_info.mem_offset = 0;
        struct_fields.push(field_info);
    }
    Type *type = struct_type(struct_fields);
    struct_decl->type = type;
    type->decl = struct_decl;
    type->name = struct_decl->name;
}

void Resolver::resolve_enum(Ast_Enum *enum_decl) {
    Auto_Array<Enum_Field_Info> enum_fields;
    s64 value = 0;
    for (int i = 0; i < enum_decl->fields.count; i++) {
        Ast_Enum_Field *field = enum_decl->fields[i];
        if (field->expr) {
            resolve_expr(field->expr);
            if (field->expr->is_constant()) {
                value = field->expr->eval.int_val;
            } else {
                report_ast_error(field, "expression does not resolve to a constant.\n");
            }
        }

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
        field->value = value;

        value++;
    }

    Enum_Type *type = enum_type(enum_fields);
    enum_decl->type = type;
    type->decl = enum_decl;
    type->name = enum_decl->name;
    type->base = type_i32;
}

void Resolver::resolve_var(Ast_Var *var) {
    if (var->init && var->init->kind == AST_COMPOUND_LITERAL) {
        Ast_Compound_Literal *literal = static_cast<Ast_Compound_Literal*>(var->init);
        literal->visited = true;

        Auto_Array<Type*> literal_types;
        for (int i = 0; i < literal->elements.count; i++) {
            Ast_Expr *element = literal->elements[i];
            resolve_expr(element);
            if (!literal_types.find(element->type)) {
                literal_types.push(element->type);
            }
        }

        Type *literal_type_spec = resolve_type(literal->type_defn);
        Type *specified_type = resolve_type(var->type_defn);

        if (specified_type && literal_type_spec) {
            if (typecheck(specified_type, literal_type_spec)) {
                var->type = specified_type;
                literal->type = specified_type;
            } else {
                report_ast_error(literal, "type mismatch, cannot convert from '%s' to '%s'.\n", string_from_type(literal_type_spec), string_from_type(specified_type));
                literal->poison();
                goto ERROR_BLOCK;
            }
        }

        Type *type = specified_type;
        if (!type) {
            type = literal_type_spec;
        }

        var->type = type;

        if (type) {
            if (type->is_array_type()) {
                Type *base_type = type->base;
                for (int i = 0; i < literal->elements.count; i++) {
                    Ast_Expr *element = literal->elements[i];
                    if (!typecheck(element->type, base_type)) {
                        report_ast_error(literal, "cannot convert from '%s' to '%s'.\n", string_from_type(element->type), string_from_type(base_type));
                        literal->poison();
                        goto ERROR_BLOCK;
                    }
                }
            } else if (type->is_struct_type()) {
                int count = (int)Min(type->aggregate.fields.count, literal->elements.count);
                for (int i = 0; i < count; i++) {
                    Ast_Expr *element = literal->elements[i];
                    Struct_Field_Info field = type->aggregate.fields[i];
                    if (!typecheck(field.type, element->type)) {
                        report_ast_error(literal, "cannot convert from '%s' to '%s'.\n", string_from_type(element->type), string_from_type(field.type));
                        literal->poison();
                        goto ERROR_BLOCK;
                    }
                }
            }
        } else {
            if (literal_types.count == 1) {
                Type *type = array_type(literal_types[0]);
                literal->type = type;
                var->type = type;
            } else {
                report_ast_error(literal, "cannot infer type of compound literal.\n");
                literal->poison();
                goto ERROR_BLOCK;
            }
        }

        if (var->type->is_array_type()) {
            Array_Type *array_type = static_cast<Array_Type*>(var->type);
            array_type->array_size = literal->elements.count;
        }
    } else {
        Type *specified_type = NULL;
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
            var->type = specified_type;
            if (var->init->valid() && !typecheck(specified_type, var->init->type)) {
                report_ast_error(var->init, "cannot assign '%s' of type '%s' to '%s'\n", string_from_expr(var->init), string_from_type(var->init->type), var->name->data);
                goto ERROR_BLOCK;
            }
        } else if (var->type_defn != NULL) {
            var->type = specified_type;
        } else {
            var->type = var->init->type;
        }

        if (var->init && var->init->kind == AST_NULL) {
            var->init->type = specified_type;
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
        Type *type = resolve_type(param->type_defn);
        param->type = type;
    }
}

void Resolver::register_global_declarations() {
    Ast_Root *root = parser->root;

    global_scope = new_scope(SCOPE_GLOBAL);
    root->scope = global_scope;

    for (Type *type : g_builtin_types) {
        if (type->name) {
            Ast_Type_Decl *decl = ast_type_decl(type->name, type);
            global_scope->declarations.push(decl);
            decl->resolve_state = RESOLVE_DONE; //@Note Don't resolve for builtin
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

void Resolver::resolve_type_decl(Ast_Type_Decl *type_decl) {
    Type *type = resolve_type(type_decl->type_defn);
    type_decl->type = type;
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
    case AST_TYPE_DECL:
    {
        Ast_Type_Decl *type_decl = static_cast<Ast_Type_Decl*>(decl);
        resolve_type_decl(type_decl);
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
        Proc_Type *proc_type = static_cast<Proc_Type*>(proc->type);
        Proc_Type *other_type = static_cast<Proc_Type*>(other->type);
        if (proc_type->parameters.count == other_type->parameters.count) {
            b32 mismatch = false;
            for (int p = 0; p < proc_type->parameters.count; p++) {
                Type *p0 = proc_type->parameters[p];
                Type *p1 = other_type->parameters[p];

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
