#include <unordered_set>

Resolver::Resolver(Parser *_parser) {
    arena = arena_create();
    parser = _parser;
}

bool Resolver::in_global_scope() {
    return current_scope == global_scope;
}

Ast_Decl *Resolver::lookup_local(Atom *name) {
    Ast_Decl *result = NULL;
    Scope *scope = current_scope;
    for (int i = 0;i < scope->declarations.count; i++) {
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
            Proc_Type *proc_type = static_cast<Proc_Type*>(proc->inferred_type);
            if (proc->op == op && typecheck(proc_type->params[0], lhs) && typecheck(proc_type->params[1], rhs)) {
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
            Proc_Type *proc_type = static_cast<Proc_Type*>(proc->inferred_type);
            if (proc->op == op && typecheck(proc_type->params[0], type)) {
                result = proc;
                break;
            }
        }
    }
    return result;
}

Ast_Decl *Resolver::lookup_overloaded(Atom *name, Auto_Array<Ast*> arguments, bool *overloaded) {
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
            Proc_Type *proc_type = static_cast<Proc_Type*>(proc->inferred_type);
            if (proc_type->params.count == arguments.count) {
                bool match = true;
                for (int j = 0; j < arguments.count; j++) {
                    Type *param = proc_type->params[j];
                    Ast *arg = arguments[j];
                    if (!typecheck(param, arg->inferred_type)) {
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

internal Struct_Field_Info *struct_lookup(Type *type, Atom *name) {
    Struct_Type *struct_type = (Struct_Type*)type;
    for (int i = 0; i < struct_type->fields.count; i++) {
        Struct_Field_Info *field = &struct_type->fields[i];
        if (atoms_match(field->name, name)) {
            return field;
        }
    }
    return NULL;
}

internal Ast_Enum_Field *enum_lookup(Ast_Enum *enum_decl, Atom *name) {
    Ast_Enum_Field *result = NULL;
    // for (int i = 0; i < enum_decl->fields.count; i++) {
    //     Ast_Enum_Field *field = enum_decl->fields[i];
    //     if (atoms_match(field->name, name)) {
    //         result = field;
    //         break;
    //     }
    // }
    return result;
}

void Resolver::resolve_if_stmt(Ast_If *if_stmt) {
    Ast *cond = if_stmt->cond;
    resolve_expr(cond);

    if (cond &&
        cond->valid() &&
        cond->inferred_type->is_struct_type()) {
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
//     Proc_Type *proc_type = static_cast<Proc_Type*>(current_proc->inferred_type);
//     current_proc->returns = true;

//     if (return_stmt->expr) {
//         resolve_expr(return_stmt->expr);

//         if (proc_type->return_type == type_void) {
//             report_ast_error(return_stmt->expr, "returning value on a void procedure.\n");
//             return_stmt->poison();
//         }

//         if (return_stmt->expr->valid() &&
//             !typecheck(proc_type->return_type, return_stmt->expr->inferred_type)) {
//             report_ast_error(return_stmt->expr, "invalid return type '%s', procedure returns '%s'.\n", string_from_type(return_stmt->expr->inferred_type), string_from_type(proc_type->return_type)); 
//             return_stmt->poison();
//         }
//     } else {
//         if (proc_type->return_type != type_void) {
//             report_ast_error(return_stmt, "procedure expects a return type '%s'.\n", string_from_type(proc_type->return_type));
//             return_stmt->poison();
//         }
//     }
}

void Resolver::resolve_fallthrough_stmt(Ast_Fallthrough *fallthrough) {
    if (!fallthrough->target) {
        report_ast_error(fallthrough, "illegal fallthrough, must be placed at end of a case block.\n");
    }
}

void Resolver::resolve_decl_stmt(Ast_Decl *decl) {
    Ast_Decl *found = lookup_local(decl->name);
    if (found == NULL) {
        if (current_scope->parent && (current_scope->parent->scope_flags & SCOPE_PROC)) {
            found = lookup(current_scope->parent, decl->name);
            if (found) {
                report_redeclaration(decl);
            }
        }

        resolve_decl(decl);
        add_entry(decl);
    } else {
        report_redeclaration(decl);
        decl->poison();
    }
}

void Resolver::resolve_while_stmt(Ast_While *while_stmt) {
    Ast *cond = while_stmt->cond;
    resolve_expr(cond);

    if (cond->valid() && !cond->inferred_type->is_conditional_type()) {
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

    ifcase->switchy = true;

    if (ifcase->cond) {
        if (!ifcase->cond->inferred_type->is_integral_type()) {
            ifcase->switchy = false;
        }
    } else {
        ifcase->switchy = false;
    }

    breakcont_stack.push(ifcase);

    for (Ast_Case_Label *case_label : ifcase->cases) {
        resolve_expr(case_label->cond);

        if (case_label->cond) {
            if (!case_label->cond->is_constant()) {
                ifcase->switchy = false;
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
                if (!typecheck(ifcase->cond->inferred_type, case_label->cond->inferred_type)) {
                    report_ast_error(case_label->cond, "'%s' is illegal type for case expression.\n", string_from_type(case_label->cond->inferred_type));
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

    if (ifcase->switchy) {
        std::unordered_set<u64> enum_values;
        for (Ast_Case_Label *label : ifcase->cases) {
            if (label->cond && label->cond->kind == AST_RANGE) {
                Ast_Range *range = static_cast<Ast_Range*>(label->cond);
                u64 min = range->lhs->eval.int_val;
                u64 max = range->rhs->eval.int_val;
                for (u64 c = min; c <= max; c++) {
                    if (enum_values.find(c) != enum_values.end()) {
                        ifcase->poison();
                        report_ast_error(label, "case value '%llu' in range '%s' already used.\n", c, string_from_expr(label->cond));
                        break;
                    } else {
                        enum_values.insert(c);
                    }
                }
            } else if (label->cond) {
                u64 c = label->cond->eval.int_val;
                auto find = enum_values.find(c);
                if (find != enum_values.end()) {
                    report_ast_error(label, "case value '%llu' already used.\n", c);
                    ifcase->poison();
                } else {
                    enum_values.insert(c);
                }
            }
        }

        if (ifcase->check_enum_complete && ifcase->cond->inferred_type->is_enum_type()) {
            Auto_Array<Ast_Enum_Field*> unused;
            Ast_Enum *enum_decl = static_cast<Ast_Enum*>(ifcase->cond->inferred_type->decl);
            // for (Ast_Enum_Field *field : enum_decl->fields) {
            //     if (enum_values.find(field->value) == enum_values.end()) {
            //         unused.push(field);
            //     }
            // }

            if (!unused.empty()) {
                report_ast_error(ifcase, "unhandled ifcase enumerations:\n");
                for (Ast_Enum_Field *field : unused) {
                    report_line("\t%s", (char *)field->name->data);
                }
            }
        }
    }
}

void Resolver::resolve_stmt(Ast_Stmt *stmt) {
    if (stmt == NULL) return;

    switch (stmt->kind) {
    case AST_EXPR_STMT: {
        Ast_Expr_Stmt *expr_stmt = (Ast_Expr_Stmt*)stmt;
        resolve_expr(expr_stmt->expr);
        break;
    }
    case AST_DECL_STMT:
        Assert(0);
        break;

    case AST_IFCASE: {
        Ast_Ifcase *ifcase = static_cast<Ast_Ifcase*>(stmt);
        resolve_ifcase_stmt(ifcase);
        break;
    }

    case AST_IF: {
        Ast_If *if_stmt = static_cast<Ast_If*>(stmt);
        for (Ast_If *node = if_stmt; node; node = (Ast_If *)node->next) {
            resolve_if_stmt(node);
        }
        break;
    }

    case AST_WHILE: {
        Ast_While *while_stmt = static_cast<Ast_While*>(stmt);
        resolve_while_stmt(while_stmt);
        break;
    }
    case AST_FOR: {
        Ast_For *for_stmt = static_cast<Ast_For*>(stmt);
        resolve_for_stmt(for_stmt);
        break;
    }
    case AST_BLOCK: {
        Ast_Block *block = static_cast<Ast_Block*>(stmt);
        resolve_block(block);
        break;
    }
    case AST_BREAK: {
        Ast_Break *break_stmt = static_cast<Ast_Break*>(stmt);
        resolve_break_stmt(break_stmt);
        break;
    }
    case AST_CONTINUE: {
        Ast_Continue *continue_stmt = static_cast<Ast_Continue*>(stmt);
        resolve_continue_stmt(continue_stmt);
        break;
    }
    case AST_RETURN: {
        Ast_Return *return_stmt = static_cast<Ast_Return*>(stmt);
        resolve_return_stmt(return_stmt);
        break;
    }
    case AST_FALLTHROUGH: {
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

    for (Ast *stmt : block->statements) {
        if (stmt->is_stmt()) {
            resolve_stmt((Ast_Stmt *)stmt);
        }
        if (stmt->is_decl()) {
            resolve_decl_stmt((Ast_Decl *)stmt);
        }
        if (stmt->kind == AST_BLOCK) {
            Ast_Block *b = static_cast<Ast_Block*>(stmt);
            if (b->returns) block->returns = true;
        } else if (stmt->kind == AST_RETURN) block->returns = true;
    }

    exit_scope();
}

// Type *Resolver::resolve_type(Ast_Type_type *type_defn) {
//     Type *type = NULL;
//     for (Ast_Type_type *t = type_defn; t; t = t->base) {
//         switch (t->type_kind) {
//         case TYPE_DEFN_NIL:
//             Assert(0);
//             break;
//         case TYPE_DEFN_NAME:
//         {
//             Ast_Decl *decl = lookup(t->name);
//             if (decl) {
//                 resolve_decl(decl);
//                 if (type) type->base = decl->inferred_type;
//                 else type = decl->inferred_type;
//             } else {
//                 report_ast_error(t, "undeclared type '%s'.\n", t->name->data);
//                 return type_poison;
//             }
//             break;
//         }

//         case TYPE_DEFN_POINTER:
//         {
//             Type *ptr = pointer_type(type);
//             type = ptr;
//             break;
//         }

//         case TYPE_DEFN_ARRAY:
//         {
//             Ast *array_size = t->array_size;
//             Array_Type *array = array_type(type);

//             if (array_size) {
//                 resolve_expr(array_size);
//                 if (array_size->inferred_type->is_integral_type()) {
//                     if (array_size->is_constant()) {
//                         array->array_size = array_size->eval.int_val;
//                         array->is_fixed = true;
//                     } else {
//                         array->is_dynamic = true;
//                     }
//                 } else {
//                     report_ast_error(array_size, "array size is not of integral type.\n");
//                     return type_poison;
//                 }
//             }

//             type = array;
//             break;
//         }

//         case TYPE_DEFN_PROC:
//         {
//             Proc_Type *proc_ty = AST_NEW(Proc_Type);
//             proc_ty->id = TYPEID_PROC;
//             for (Ast_Type_type *param : t->proc.params) {
//                 Type *param_ty = resolve_type(param);
//                 proc_ty->params.push(param_ty);
//             }
//             if (t->proc.return_type) {
//                 proc_ty->return_type = resolve_type(t->proc.return_type);
//             } else {
//                 proc_ty->return_type = type_void;
//             }
//             type = proc_ty;
//             break;
//         }
//         }
//     }
//     return type;
// }

void Resolver::resolve_assignment_expr(Ast_Assignment *assignment) {
//     Ast *lhs = assignment->lhs;
//     Ast *rhs = assignment->rhs;

//     resolve_expr(lhs);
//     resolve_expr(rhs);

//     if (lhs->valid() && !(lhs->flags & AST_FLAG_LVALUE)) {
//         report_ast_error(lhs, "cannot assign to '%s', is not an l-value.\n", string_from_expr(lhs));
//         assignment->poison();
//     }

//     if (lhs->valid() && rhs->valid()) {
//         if (!typecheck(lhs->inferred_type, rhs->inferred_type)) {
//             report_ast_error(rhs, "type mismatch, cannot assign to '%s' from '%s'.\n", string_from_type(lhs->inferred_type), string_from_type(rhs->inferred_type));
//             assignment->poison();
//         }
//     }
//     assignment->inferred_type = lhs->inferred_type;
}

void Resolver::resolve_builtin_unary_expr(Ast_Unary *expr) {
    Ast *elem = expr->elem;
    switch (expr->op) {
    case OP_UNARY_PLUS:
        if (elem->inferred_type->is_numeric_type()) {
            expr->inferred_type = elem->inferred_type;
        } else {
            report_ast_error(elem, "invalid operand '%s' of type '%s' in unary '%s'.\n", string_from_expr(elem), string_from_type(elem->inferred_type), string_from_operator(expr->op));
            expr->poison();
        }
        break;
    case OP_UNARY_MINUS:
        if (elem->inferred_type->is_numeric_type() && !elem->inferred_type->is_pointer_type()) {
            expr->inferred_type = elem->inferred_type;
        } else {
            report_ast_error(expr, "invalid operand '%s' of type '%s' in unary '%s'.\n", string_from_expr(elem), string_from_type(elem->inferred_type), string_from_operator(expr->op));
            expr->poison();
        }
        break;
    case OP_NOT:
        if (elem->inferred_type->is_numeric_type()) {
            expr->inferred_type = type_bool;
        } else {
            report_ast_error(expr, "invalid operand '%s' of type '%s' in unary '%s'.\n", string_from_expr(elem), string_from_type(elem->inferred_type), string_from_operator(expr->op));
            expr->poison();
        }
        break;
    case OP_BIT_NOT:
        if (elem->inferred_type->is_numeric_type()) {
            expr->inferred_type = elem->inferred_type;
        } else {
            report_ast_error(expr, "invalid operand '%s' of type '%s' in unary '%s'.\n", string_from_expr(elem), string_from_type(elem->inferred_type), string_from_operator(expr->op));
            expr->poison();
        }
        break;
    }
}

void Resolver::resolve_binary_expr(Ast_Binary *expr) {
    Ast *lhs = expr->lhs;
    Ast *rhs = expr->rhs;

    resolve_expr(lhs);
    resolve_expr(rhs);

    if (lhs->invalid()) {
        expr->poison();
    }
    if (rhs->invalid()) {
        expr->poison();
    }

    if (lhs->valid() && rhs->valid()) {
        if (lhs->inferred_type->is_user_defined_type() || rhs->inferred_type->is_user_defined_type()) {
            Ast_Operator_Proc *proc = lookup_user_defined_binary_operator(expr->op, expr->lhs->inferred_type, expr->rhs->inferred_type);
            if (proc) {
                resolve_proc_header(proc);
                expr->proc = proc;
                expr->flags |= AST_FLAG_OP_CALL;
                if (proc->valid()) {
                    // expr->inferred_type = static_cast<Proc_Type*>(proc->inferred_type)->return_type;
                }
            } else {
                report_ast_error(expr, "could not find operator'%s' (%s,%s).\n", string_from_operator(expr->op), string_from_type(expr->lhs->inferred_type), string_from_type(expr->rhs->inferred_type));
                expr->poison();
            }
        } else {
            resolve_builtin_binary_expr(expr);  
        }
    }
}

void Resolver::resolve_cast_expr(Ast_Cast *cast) {
    resolve_expr(cast->elem);
    cast->inferred_type = resolve_type(cast->type);

    if (cast->elem->valid()) {
        if (!typecheck_castable(cast->inferred_type, cast->elem->inferred_type)) {
            report_ast_error(cast->elem, "cannot cast '%s' as '%s' from '%s'.\n", string_from_expr(cast->elem), string_from_type(cast->inferred_type), string_from_type(cast->elem->inferred_type));
        }
    } else {
        cast->poison();
    }
}

void Resolver::resolve_subscript_expr(Ast_Subscript *subscript) {
    resolve_expr(subscript->expr);
    resolve_expr(subscript->index);

    if (subscript->expr->valid()) {
        if (subscript->expr->inferred_type->is_indirection_type()) {
            subscript->inferred_type = subscript->expr->inferred_type->base;
        } else {
            report_ast_error(subscript->expr, "'%s' is not a pointer or array type.\n", string_from_expr(subscript->expr));
            subscript->poison();
        }
    } else {
        subscript->poison();
    }

    if (subscript->index->valid()) {
        if (!subscript->index->inferred_type->is_integral_type()) {
            report_ast_error(subscript->index, "array subscript is not of integral type.\n");
            subscript->poison();
        }
    } else {
        subscript->poison();
    }
}

void Resolver::resolve_compound_literal(Ast_Compound_Literal *literal) {
    Type *specified_type = resolve_type(literal->type);

    literal->inferred_type = specified_type;

    for (int i = 0; i < literal->elements.count; i++) {
        Ast *elem = literal->elements[i];
        resolve_expr(elem);

    }

    if (specified_type->is_struct_type()) {
        Struct_Type *struct_type = (Struct_Type *)specified_type;
        if (struct_type->fields.count < literal->elements.count) {
            report_ast_error(literal, "too many initializers for struct '%s'.\n", struct_type->decl->name->data);
            report_note(struct_type->decl->start, "see declaration of '%s'.\n", struct_type->decl->name->data);
            literal->poison();
        }

        int elem_count = Min((int)struct_type->fields.count, (int)literal->elements.count);
        for (int i = 0; i < elem_count; i++) {
            Ast *elem = literal->elements[i];
            if (elem->invalid()) continue;

            Struct_Field_Info *field = &struct_type->fields[i];
            if (!typecheck(field->type, elem->inferred_type)) {
                report_ast_error(elem, "cannot convert from '%s' to '%s'.\n", string_from_type(elem->inferred_type), string_from_type(field->type));
                literal->poison();
            }
        }
    } else if (specified_type->is_array_type()) {
        Type *elem_type = specified_type->base;
        for (int i = 0; i < literal->elements.count; i++) {
            Ast *elem = literal->elements[i];
            if (elem->invalid()) break;

            if (!typecheck(elem_type, elem->inferred_type)) {
                report_ast_error(elem, "cannot convert from '%s' to '%s'.\n", string_from_type(elem->inferred_type), string_from_type(elem_type));
                literal->poison();
            }
        }
    }

    bool is_constant = true;
    for (int i = 0; i < literal->elements.count; i++) {
        Ast *elem = literal->elements[i];
        if (!elem->is_constant()) {
            is_constant = false;
        }
    }
    if (is_constant) {
        literal->flags |= AST_FLAG_CONSTANT;
    }
}

internal Eval eval_from_decl(Ast_Decl *decl) {
    Eval eval = {};
    switch (decl->kind) {
    case AST_ENUM_FIELD: {
        Ast_Enum_Field *field = (Ast_Enum_Field *)decl;
        eval.int_val = field->value;
        return eval;
    }
    case AST_VAR: {
        Ast_Var *var = (Ast_Var *)decl;
        return var->init->eval;
    }
    }
    Assert(0);
    return eval;
}

void Resolver::resolve_ident(Ast_Ident *ident) {
    Ast_Decl *ref = lookup(ident->name);

    if (ref) {
        resolve_decl(ref);
        ident->ref = ref;
        ident->inferred_type = ref->inferred_type;

        if (!(ref->flags & AST_FLAG_TYPE) && !(ref->flags & AST_FLAG_CONSTANT)) {
            ident->flags |= AST_FLAG_LVALUE;
        }

        if (ref->flags & AST_FLAG_CONSTANT) {
            ident->flags |= AST_FLAG_CONSTANT;
            ident->eval = eval_from_decl(ref);
        }

        if (ref->invalid()) ident->poison();
    } else {
        report_undeclared(ident);
        ident->poison();
    }
}

void Resolver::resolve_call_expr(Ast_Call *call) {
    for (int i = 0; i < call->arguments.count; i++) {
        Ast *arg = call->arguments[i];
        resolve_expr(arg);
        if (arg->invalid()) call->poison();
    }

    Ast *elem = call->elem;

    Ast_Proc *proc = nullptr;

    if (call->elem->kind == AST_IDENT) {
        Ast_Proc *proc = nullptr;
        Ast_Ident *name = static_cast<Ast_Ident*>(call->elem);
        bool overloaded = false;
        Ast_Decl *decl = lookup_overloaded(name->name, call->arguments, &overloaded);
        if (decl) {
            resolve_decl(decl);
            name->ref = decl;
            elem->inferred_type = decl->inferred_type;
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

    if (elem->inferred_type->is_proc_type()) {
        proc_type = static_cast<Proc_Type*>(elem->inferred_type);
    } else {
        report_ast_error(call, "'%s' does not evaluate to a procedure.\n", string_from_expr(call->elem));
        call->poison();
        return;
    }

    if ((proc_type->has_varargs && (call->arguments.count < proc_type->params.count - 1))
        || (!proc_type->has_varargs && (call->arguments.count != proc_type->params.count))) {
        report_ast_error(call, "'%s' does not take %d arguments.\n", string_from_expr(elem), call->arguments.count);
        if (proc) {
            report_note(proc->start, "see declaration of '%s'.\n", proc->name->data);
        }
        call->poison();
        return;
    }

    bool bad_arg = false;
    for (int i = 0, param_idx = 0; i < call->arguments.count; i++) {
        Type *param_type = proc_type->params[param_idx];
        Ast *arg = call->arguments[i];
        if (param_type == NULL) { //@Note @Fix this is a vararg type
            if (arg->valid() && !typecheck(param_type, arg->inferred_type)) {
                bad_arg = true;
                report_ast_error(arg, "incompatible argument of type '%s' for parameter of type '%s'.\n", string_from_type(arg->inferred_type), string_from_type(param_type));
                call->poison();
            }
            param_idx++;
        }
    }
    if (proc && bad_arg) {
        report_note(proc->start, "see declaration of '%s'.\n", proc->name->data);
        call->poison();
    }

    // call->inferred_type = proc_type->return_type;
}

void Resolver::resolve_deref_expr(Ast_Deref *deref) {
    resolve_expr(deref->elem);
    if (deref->elem->inferred_type->is_indirection_type()) {
        deref->inferred_type = deref->elem->inferred_type->base;
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
    
    if (access->parent->inferred_type->is_struct_or_pointer()) {
        access->flags |= AST_FLAG_LVALUE;

        Type *struct_type = access->parent->inferred_type;
        if (access->parent->inferred_type->is_pointer_type()) {
            struct_type = access->parent->inferred_type->base;
        }

        Struct_Field_Info *struct_field = struct_lookup(struct_type, access->name->name);
        if (!struct_field) {
            report_ast_error(access->parent, "'%s' is not a member of '%s'.\n", access->name->name->data, struct_type->decl->name->data);
            access->poison();
            return;
        }
        access->inferred_type = struct_field->type;
    } else if (access->parent->inferred_type->is_enum_type()) {
        if (access->parent->kind == AST_ACCESS) {
            Ast_Access *parent_access = static_cast<Ast_Access*>(access->parent);
            if (parent_access->parent->inferred_type->is_struct_type()) {
                report_ast_error(access, "cannot access expression of enum inside struct.\n");
                access->poison();
                return;
            }
        }

        Enum_Type *enum_type = static_cast<Enum_Type*>(access->parent->inferred_type);
        Ast_Enum_Field *enum_field = enum_lookup((Ast_Enum *)enum_type->decl, access->name->name);
        if (!enum_field) {
            report_ast_error(access->parent, "'%s' is not a member of '%s'.\n", access->name->name->data, enum_type->decl->name->data);
            access->poison();
            return;
        }
        access->inferred_type = access->parent->inferred_type;
        access->eval.int_val = enum_field->value;
        access->flags |= AST_FLAG_CONSTANT;
    } else if (access->parent->inferred_type->is_array_type()) {
        access->flags |= AST_FLAG_LVALUE;

        Type *type = access->parent->inferred_type;
        Struct_Field_Info *struct_field = struct_lookup(type, access->name->name);
        if (!struct_field) {
            report_ast_error(access->parent, "'%s' is not a member of array type.\n", access->name->name->data);
            access->poison();
            return;
        }
        access->inferred_type = struct_field->type;
    } else if (access->parent->inferred_type->id == TYPEID_STRING) {
        access->flags |= AST_FLAG_LVALUE;

        Type *type = access->parent->inferred_type;
        Struct_Field_Info *struct_field = struct_lookup(type, access->name->name);
        if (!struct_field) {
            report_ast_error(access->parent, "'%s' is not a member of string type.\n", access->name->name->data);
            access->poison();
            return;
        }
        access->inferred_type = struct_field->type;
    } else {
        report_ast_error(access->parent, "cannot access left of .'%s', illegal type '%s'.\n", access->name->name->data, string_from_type(access->parent->inferred_type));
        access->poison();
        return;
    }
}

void Resolver::resolve_range_expr(Ast_Range *range) {
    resolve_expr(range->lhs);
    resolve_expr(range->rhs);

    if (range->lhs->valid()) {
        if (!range->lhs->inferred_type->is_integral_type()) {
            report_ast_error(range->lhs, "'%s' is invalid range expression, not an integral type.\n", string_from_expr(range->lhs));
            range->poison();
        }
    }

    if (range->rhs->valid()) {
        if (!range->rhs->inferred_type->is_integral_type()) {
            report_ast_error(range->rhs, "'%s' is invalid range expression, not an integral type.\n", string_from_expr(range->rhs));
            range->poison();
        }
    }

    if (range->lhs->valid() && range->rhs->valid()) {
        if (typecheck(range->lhs->inferred_type, range->rhs->inferred_type)) {
            
        } else {
            report_ast_error(range, "mismatched types in range expression ('%s' and '%s').\n", string_from_type(range->lhs->inferred_type), string_from_type(range->rhs->inferred_type));
        }
    }

    if (range->lhs->is_constant() && range->rhs->is_constant()) range->flags |= AST_FLAG_CONSTANT;

    range->inferred_type = range->lhs->inferred_type;
}

void Resolver::resolve_address_expr(Ast_Address *address) {
    resolve_expr(address->elem);

    if (address->elem->valid()) {
        address->inferred_type = pointer_type_create(address->elem->inferred_type);

        if (!(address->elem->flags & AST_FLAG_LVALUE)) {
            report_ast_error(address->elem, "cannot take address of '%s'.\n", string_from_expr(address->elem));
            address->poison();
        }
    } else {
        address->poison();
    }
}

void Resolver::resolve_expr(Ast *expr) {
    if (expr == NULL) return;

    if (expr->visited) return;

    switch (expr->kind) {
    default:
        Assert(0);
        break;

    }

    expr->visited = true;
}

// void Resolver::resolve_proc_header(Ast_Proc *proc) {
//     Type *return_type = resolve_type(proc->return_type);
//     Auto_Array<Type*> params;
//     bool has_varargs = false;
//     for (int i = 0; i < proc->params.count; i++) {
//         Ast_Param *param = proc->params[i];
//         if (param->is_vararg) has_varargs = true;
//         Type *type = resolve_type(param->type);
//         param->inferred_type = type;
//         params.push(type);
//     }
//     if (return_type == NULL) return_type = type_void;

//     Proc_Type *type = proc_type_create(return_type, params);
//     type->has_varargs = has_varargs;
//     type->decl = proc;
//     proc->inferred_type = type;

//     if (proc->kind == AST_OPERATOR_PROC) {
//         Ast_Operator_Proc *operator_proc = static_cast<Ast_Operator_Proc*>(proc);
//         Proc_Type *type = static_cast<Proc_Type*>(operator_proc->inferred_type);

//         if (type->params.count == 0) {
//             report_ast_error(proc, "operator %s missing params.\n", string_from_operator(operator_proc->op));
//         }

//         bool has_custom_type = false;
//         for (int i = 0; i < type->params.count; i++) {
//             Type *param = type->params[i];
//             if (param->is_user_defined_type()) {
//                 has_custom_type = true;
//             }
//         }

//         if (!has_custom_type) {
//             report_ast_error(proc, "operator %s must have at least one user-defined type.\n", string_from_operator(operator_proc->op));
//         }
//     }
// }

// void Resolver::resolve_proc(Ast_Proc *proc) {
//     resolve_proc_header(proc);
//     if (in_global_scope() && proc->block) {
//         Scope *scope = new_scope(SCOPE_PROC);
//         proc->scope = scope;
//         current_proc = proc;
//         for (int i = 0; i < proc->params.count; i++) {
//             Ast_Param *param = proc->params[i];
//             add_entry(param);
//             proc->local_vars.push(param);
//         }
//         resolve_block(proc->block);
//         exit_scope();
//     }
// }

// void Resolver::resolve_struct(Ast_Struct *struct_node) {
//     struct_node->scope = new_scope(SCOPE_STRUCT);

//     Auto_Array<Struct_Field_Info> struct_fields;
//     for (Ast_Decl *member : struct_node->members) {
//         resolve_decl(member);

//         add_entry(member);

//         if (member->kind == AST_VAR) {
//             Struct_Field_Info field_info = struct_field_info(member->name, member->inferred_type);
//             struct_fields.push(field_info);
//         }
//     }

//     Type *type = struct_type_create(struct_fields);
//     struct_node->inferred_type = type;
//     type->decl = struct_node;
//     type->name = struct_node->name;

//     exit_scope();
// }

// void Resolver::resolve_enum(Ast_Enum *enum_decl) {
//     enum_decl->scope = new_scope(SCOPE_ENUM);
//     Auto_Array<Enum_Field_Info> enum_fields;
//     s64 value = 0;
//     // for (Ast_Enum_Field *field : enum_decl->fields) {
//     //     add_entry(field);
//     //     if (field->expr) {
//     //         resolve_expr(field->expr);
//     //         if (field->expr->is_constant()) {
//     //             value = field->expr->eval.int_val;
//     //         } else {
//     //             report_ast_error(field->expr, "expression does not resolve to a constant.\n");
//     //         }
//     //     }

//     //     for (int j = 0; j < enum_fields.count; j++) {
//     //         Enum_Field_Info field_info = enum_fields[j];
//     //         if (atoms_match(field->name, field_info.name)) {
//     //             report_ast_error(field, "'%s' is already declared in enum.\n", field->name->data);
//     //             break;
//     //         }
//     //     }

//     //     Enum_Field_Info field_info = {};
//     //     field_info.name = field->name;
//     //     field_info.value = value;
//     //     enum_fields.push(field_info);
//     //     field->value = value;

//     //     value++;
//     // }

//     Enum_Type *enum_type = enum_type_create(type_i32);
//     enum_decl->inferred_type = enum_type;
//     enum_type->name = enum_decl->name;
//     enum_type->decl = enum_decl;

//     exit_scope();
// }

// void Resolver::resolve_var(Ast_Var *var) {
//     if (var->init && var->init->kind == AST_COMPOUND_LITERAL) {
//         Ast_Compound_Literal *literal = static_cast<Ast_Compound_Literal*>(var->init);
//         literal->visited = true;

//         Auto_Array<Type*> literal_types;
//         for (int i = 0; i < literal->elements.count; i++) {
//             Ast *element = literal->elements[i];
//             resolve_expr(element);
//             if (!literal_types.find(element->inferred_type)) {
//                 literal_types.push(element->inferred_type);
//             }
//         }

//         Type *literal_type_spec = resolve_type(literal->type);
//         Type *specified_type = resolve_type(var->type);

//         if (specified_type && literal_type_spec) {
//             if (typecheck(specified_type, literal_type_spec)) {
//                 var->inferred_type = specified_type;
//                 literal->inferred_type = specified_type;
//             } else {
//                 report_ast_error(literal, "type mismatch, cannot convert from '%s' to '%s'.\n", string_from_type(literal_type_spec), string_from_type(specified_type));
//                 literal->poison();
//                 goto ERROR_BLOCK;
//             }
//         }

//         Type *type = specified_type;
//         if (!type) {
//             type = literal_type_spec;
//         }

//         var->inferred_type = type;

//         if (type) {
//             if (type->is_array_type()) {
//                 Type *base_type = type->base;
//                 for (int i = 0; i < literal->elements.count; i++) {
//                     Ast *element = literal->elements[i];
//                     if (!typecheck(element->inferred_type, base_type)) {
//                         report_ast_error(literal, "cannot convert from '%s' to '%s'.\n", string_from_type(element->inferred_type), string_from_type(base_type));
//                         literal->poison();
//                         goto ERROR_BLOCK;
//                     }
//                 }
//             } else if (type->is_struct_type()) {
//                 Struct_Type *struct_type = (Struct_Type *)type;
//                 int count = (int)Min(struct_type->fields.count, literal->elements.count);
//                 for (int i = 0; i < count; i++) {
//                     Ast *element = literal->elements[i];
//                     Struct_Field_Info field = struct_type->fields[i];
//                     if (!typecheck(field.type, element->inferred_type)) {
//                         report_ast_error(literal, "cannot convert from '%s' to '%s'.\n", string_from_type(element->inferred_type), string_from_type(field.type));
//                         literal->poison();
//                         goto ERROR_BLOCK;
//                     }
//                 }
//             }
//         } else {
//             if (literal_types.count == 1) {
//                 Type *type = array_type_create(literal_types[0]);
//                 literal->inferred_type = type;
//                 var->inferred_type = type;
//             } else {
//                 report_ast_error(literal, "cannot infer type of compound literal.\n");
//                 literal->poison();
//                 goto ERROR_BLOCK;
//             }
//         }

//         if (var->inferred_type->is_array_type()) {
//             Array_Type *array_type = static_cast<Array_Type*>(var->inferred_type);
//             array_type->array_size = literal->elements.count;
//         }
//     } else {
//         Type *specified_type = NULL;
//         if (var->type) {
//             specified_type = resolve_type(var->type);
//         }

//         if (var->init) {
//             resolve_expr(var->init);
//             if (var->init->invalid()) {
//                 goto ERROR_BLOCK;
//             }
//         }

//         if (var->type && var->init) {
//             var->inferred_type = specified_type;
//             if (var->init->valid() && !typecheck(specified_type, var->init->inferred_type)) {
//                 report_ast_error(var->init, "cannot assign '%s' of type '%s' to '%s'\n", string_from_expr(var->init), string_from_type(var->init->inferred_type), var->name->data);
//                 goto ERROR_BLOCK;
//             }
//         } else if (var->type != NULL) {
//             var->inferred_type = specified_type;
//         } else {
//             var->inferred_type = var->init->inferred_type;
//         }
//     }

//     if (!(var->flags & AST_FLAG_GLOBAL) && current_proc) {
//         current_proc->local_vars.push(var);
//     }

//     return;

// ERROR_BLOCK:
//     var->poison();
// }

// void Resolver::resolve_param(Ast_Param *param) {
//     if (!param->is_vararg) {
//         Type *type = resolve_type(param->type);
//         param->inferred_type = type;
//     }
// }

void Resolver::resolve_type_decl(Ast_Type_Decl *type_decl) {
    Type *type = resolve_type(type_decl->type);
    type_decl->inferred_type = type;
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
        Proc_Type *proc_type = static_cast<Proc_Type*>(proc->inferred_type);
        Proc_Type *other_type = static_cast<Proc_Type*>(other->inferred_type);
        if (proc_type->params.count == other_type->params.count) {
            b32 mismatch = false;
            for (int p = 0; p < proc_type->params.count; p++) {
                Type *p0 = proc_type->params[p];
                Type *p1 = other_type->params[p];

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

Decl *Resolver::lookup(Atom *name) {
    for (Scope *scope = current_scope; scope; scope = scope->parent) {
        for (int i = 0; i < scope->declarations.count; i++) {
            Decl *decl = scope->decls[i];
            if (atoms_match(decl->name, name)) {
                return decl;
            }
        }
    }
    return NULL;
}

void Resolver::register_global_declarations() {
    Ast_Root *root = parser->root;

    global_scope = new_scope(SCOPE_GLOBAL);
    root->scope = global_scope;

    for (Type *type : g_builtin_types) {
        if (type->name) {
            Decl *decl = decl_alloc();
            decl->name = type->name;
            decl->type = type;
            decl->resolve_state = RESOLVE_DONE; //@Note Don't resolve for builtin
            type->decl = decl;
            add_entry(decl);
        }
    }

    for (Ast *decl_node : root->decls) {
        switch (decl_node->kind) {
        case AST_VALUE_DECL: {
            Ast_Value_Decl *value_decl = (Ast_Value_Decl *)decl_node;
            // decl->flags |= AST_FLAG_GLOBAL;
            // if (decl->kind == AST_PROC || decl->kind == AST_OPERATOR_PROC) {
            // } else {
            // }

            for (Ast *name_node : value_decl->names) {
                Assert(ident->kind == AST_IDENT);
                Ast_Ident *ident = (Ast_Ident *)name_node;
                Decl *ref = lookup(ident->name);
                ident->ref = ref;
                if (ref) {
                    report_line("redeclaration of '%s'.\n", ident->name->data);
                    continue;
                }

                Decl *decl = decl_alloc();
                decl->name = name;
                add_entry(decl);
                ident->ref = decl;
            }
        }
        }
    }
}

void Resolver::resolve_value_decl(Ast_Value_Decl *vd) {
    
}

void Resolver::resolve_decl(Ast *decl) {
    switch (decl->kind) {
    case AST_VALUE_DECL: {
        Ast_Value_Decl *vd = (Ast_Value_Decl *)decl;
        resolve_value_decl(vd);
        break;
    }
    }
}

void Resolver::resolve() {
    Ast_Root *root = parser->root;

    register_global_declarations();

    for (Ast *decl : root->decls) {
        resolve_decl(decl);
    }

    // for (int i = 0; i < root->declarations.count; i++) {
    //     Ast_Decl *decl = root->declarations[i];
    //     if (decl->kind == AST_PROC || decl->kind == AST_OPERATOR_PROC) {
    //         Ast_Proc *proc = static_cast<Ast_Proc*>(decl);
    //         resolve_overloaded_proc(proc);
    //     }
    // }
}
