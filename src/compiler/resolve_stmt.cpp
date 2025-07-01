
void Resolver::resolve_block(Ast_Block *block) {
    block->scope = new_scope(current_scope, SCOPE_BLOCK);

    for (Ast *stmt : block->statements) {
        resolve_stmt(stmt);
    }

    exit_scope();
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
    Scope *scope = new_scope(current_scope, SCOPE_BLOCK);

    if (for_stmt->range_expr) {
        resolve_expr(for_stmt->range_expr);

        if (for_stmt->range_expr->kind != AST_RANGE && !for_stmt->range_expr->inferred_type->is_array_type()) {
            report_ast_error(for_stmt->range_expr, "'%s' of type '%s' is not iterable.\n", string_from_expr(for_stmt->range_expr), string_from_type(for_stmt->inferred_type));
        }
    }

    if (for_stmt->lhs.count > 0) {
        if (for_stmt->lhs.count > 2) {
            report_ast_error(for_stmt, "exceeded number of lhs in for-loop.\n");
        }

        for (int i = 0; i < for_stmt->lhs.count; i++) {
            Ast *name = for_stmt->lhs[i];
            if (name->kind != AST_IDENT) {
                report_ast_error(name, "'%s' found where for-loop iterator name expected.\n", string_from_expr(name));
                continue;
            }
            Ast_Ident *ident = (Ast_Ident *)name;
            Decl *decl = decl_variable_create(ident->name);
            decl->node = name;
            decl->init_expr = for_stmt->range_expr;
            decl->resolve_state = RESOLVE_DONE;
            scope_add(scope, decl);

            if (i == 0) { // index
                decl->type = type_int;
                for_stmt->index_variable = decl;
            } else { // value
                decl->type = for_stmt->range_expr->inferred_type;
                for_stmt->value_variable = decl;
            }
            ident->ref = decl;
        }
    }


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
            if (case_label->cond->mode != ADDRESSING_CONSTANT) {
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
            if (case_label->cond && !is_convertible(ifcase->cond->inferred_type, case_label->cond->inferred_type)) {
                report_ast_error(case_label->cond, "'%s' is illegal type for case expression.\n", string_from_type(case_label->cond->inferred_type));
                ifcase->poison();
            }
        } else {
            if (case_label->cond && case_label->cond->kind == AST_RANGE) {
                report_ast_error(case_label, "illegal range condition in an ifcase without an initial condition to compare.\n");
                ifcase->poison();
            }
        }

        case_label->scope = new_scope(current_scope, SCOPE_BLOCK);

        for (Ast *stmt : case_label->statements) {
            resolve_stmt(stmt);
        }
        exit_scope();
    }

    breakcont_stack.pop();

    if (ifcase->switchy) {
        //@Todo @Fix Check for signedness of integer
        std::unordered_set<u64> enum_values;

        for (Ast_Case_Label *label : ifcase->cases) {
            if (label->cond && label->cond->kind == AST_RANGE) {
                Ast_Range *range = static_cast<Ast_Range*>(label->cond);
                if (!range->lhs->inferred_type->is_integral_type()) {
                    report_ast_error(range->lhs, "range in for loop must be integral type.\n");
                    continue;
                }
                if (!range->rhs->inferred_type->is_integral_type()) {
                    report_ast_error(range->rhs, "range in for loop must be integral type.\n");
                    continue;
                }
                u64 min = u64_from_bigint(range->lhs->value.value_integer);
                u64 max = u64_from_bigint(range->rhs->value.value_integer);
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
                u64 c = u64_from_bigint(label->cond->value.value_integer);
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
            Auto_Array<Decl*> unused = {};
            Enum_Type *et = (Enum_Type *)ifcase->cond->inferred_type;

            for (Decl *field : et->fields) {
                if (enum_values.find(u64_from_bigint(field->constant_value.value_integer)) == enum_values.end()) {
                    unused.push(field);
                }
            }

            if (!unused.empty()) {
                report_ast_error(ifcase, "unhandled ifcase enumerations:\n");
                for (Decl *field : unused) {
                    report_line("\t%s", (char *)field->name->data);
                }
            }
        }
    }
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
    Assert(current_proc);

    Proc_Type *proc_type = (Proc_Type *)(current_proc->inferred_type);
    Tuple_Type *results = proc_type->results;

    for (Ast *value : return_stmt->values) {
        resolve_expr(value);
    }

    int total_value_count = get_values_count(return_stmt->values);
    int proc_type_count = type_value_count(proc_type->results);

    if (total_value_count == proc_type_count) {
        for (int idx = 0, vidx = 0; vidx < total_value_count; vidx++) {
            Ast *value = return_stmt->values[vidx];
            int value_type_count = type_value_count(value->inferred_type);

            for (int i = 0; i < value_type_count; i++) {
                Type *result_type = results->types[idx];
                Type *value_type = type_from_index(value->inferred_type, i);
                if (!is_convertible(value_type, result_type)) {
                    report_ast_error(value, "cannot convert value of '%s' from '%s' to '%s'.\n", string_from_expr(value), string_from_type(value_type), string_from_type(result_type));
                }
                idx++;
            }
        }
    } else {
        report_ast_error(return_stmt, "expected '%d' return values, got '%d'.\n", proc_type_count, total_value_count);
    }
}

void Resolver::resolve_fallthrough_stmt(Ast_Fallthrough *fallthrough) {
    if (!fallthrough->target) {
        report_ast_error(fallthrough, "illegal fallthrough, must be placed at end of a case block.\n");
    }
}

void Resolver::resolve_assignment_stmt(Ast_Assignment *assign) {
    for (Ast *lhs : assign->lhs) {
        resolve_expr(lhs);
        if (lhs->valid()) {
            if (lhs->mode != ADDRESSING_VARIABLE) {
                report_ast_error(lhs, "cannot assign to lhs, not a l-value.\n");
            }
        }
    }

    for (Ast *rhs : assign->rhs) {
        resolve_expr_base(rhs);
    }
}

void Resolver::resolve_value_decl_stmt(Ast_Value_Decl *vd) {
    if (vd->is_mutable) {
        for (int i = 0; i < vd->names.count; i++) {
            Ast_Ident *ident = (Ast_Ident *)vd->names[i];
            Decl *found = scope_find(current_scope, ident->name);
            ident->ref = found;

            if (found) {
                report_ast_error(ident, "redeclaration of %s.\n", ident->name->data);
                continue;
            }

            Decl *decl = decl_variable_create(ident->name);
            decl->node = vd;
            decl->type_expr = vd->type;
            ident->ref = decl;
            // decl->init_expr = value;
            scope_add(current_scope, decl);
        }
        resolve_value_decl(vd, false);
    } else {
        for (int i = 0; i < vd->names.count; i++) {
            Ast_Ident *ident = (Ast_Ident *)vd->names[i];
            Ast *value = vd->values[i];

            Decl *found = scope_find(current_scope, ident->name);
            ident->ref = found;

            if (found) {
                char *name = get_name_scoped(ident->name, current_scope);
                report_ast_error(ident, "redeclaration of %s.\n", name);
                continue;
            }

            Decl *decl = nullptr;
            if (is_ast_type(value)) {
                decl = decl_type_create(ident->name);
            } else if (value->kind == AST_PROC_LIT) {
                decl = decl_procedure_create(ident->name);
                decl->proc_lit = (Ast_Proc_Lit *)value;
            } else {
                decl = decl_constant_create(ident->name);
            }
            decl->node = vd;
            decl->init_expr = value;
            ident->ref = decl;
            scope_add(current_scope, decl);

            resolve_decl(decl);
        }
    }
}

void Resolver::resolve_stmt(Ast *stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
    case AST_VALUE_DECL: {
        Ast_Value_Decl *vd = static_cast<Ast_Value_Decl*>(stmt);
        resolve_value_decl_stmt(vd);
        break;
    }

    case AST_EXPR_STMT: {
        Ast_Expr_Stmt *expr_stmt = (Ast_Expr_Stmt *)stmt;
        resolve_expr_base(expr_stmt->expr);
        break;
    }

    case AST_ASSIGNMENT: {
        Ast_Assignment *assign = (Ast_Assignment *)stmt;
        resolve_assignment_stmt(assign);
        break;
    }

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
