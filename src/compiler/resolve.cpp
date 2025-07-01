//@Todo Fix resolution of declarations

#include <unordered_set>

struct Select {
    Decl *decl = nullptr;
    int index = 0;
};

void Resolver::type_complete_path_add(Ast *type) {
    type_complete_path.push(type);
}

void Resolver::type_complete_path_clear() {
    type_complete_path.clear();
}

internal char *get_scoping_name(Scope *scope) {
    cstring string = nullptr;
    while (scope) {
        if (!scope->name) {
            break;
        }

        cstring_prepend(&string, scope->name->data); 

        if (scope->parent && scope->parent->name) {
            cstring_prepend(&string, ".");
        }

        scope = scope->parent;
    }
    return string;
}

internal char *get_name_scoped(Atom *name, Scope *scope) {
    char *string = make_cstring(name->data);
    if (scope->name) {
        cstring_prepend(&string, ".");
        char *scope_name = get_scoping_name(scope);
        cstring_prepend(&string, scope_name);
    }
    return string;
}

internal int type_value_count(Type *type) {
    if (!type) return 0;
    if (type->kind == TYPE_TUPLE) {
        Tuple_Type *tuple = (Tuple_Type *)type;
        return (int)tuple->types.count;
    }
    return 1;
}

internal Type *type_from_index(Type *type, int idx) {
    if (type->kind == TYPE_TUPLE) {
        Tuple_Type *tuple = (Tuple_Type *)type;
        Assert(idx < tuple->types.count);
        return tuple->types[idx];
    } else {
        Assert(idx == 0);
        return type;
    }
}

internal int get_values_count(Auto_Array<Ast*> values) {
    int count = 0;
    for (Ast *value : values) {
        Type *type = value->inferred_type;

        if (type->kind == TYPE_TUPLE) {
            count += (int)((Tuple_Type *)type)->types.count;
        } else {
            count += 1;
        }
    }
    return count;
}

Resolver::Resolver(Parser *_parser) {
    arena = arena_create();
    parser = _parser;
}

Scope *Resolver::new_scope(Scope *parent, Scope_Flags scope_flags) {
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

void Resolver::resolve_type_decl(Decl *decl) {
    Ast *type_expr = decl->init_expr;

    Type *type = resolve_type(type_expr);
    type->name = decl->name;

    decl->type = type;

    //@Note Resolve procedures after type data members so as to avoid cyclic definition error.
    decl->resolve_state = RESOLVE_DONE;
    if (type->kind == TYPE_STRUCT) {
        Struct_Type *st = (Struct_Type *)type;
        for (Decl *member : st->members) {
            if (member->kind == DECL_PROCEDURE) {
                resolve_decl(member);
            }
        }
    }
}

void Resolver::resolve_variable_decl(Decl *decl) {
    Type *type = resolve_type(decl->type_expr);
    decl->type = type;
}

void Resolver:: resolve_constant_decl(Decl *decl) {

}

void Resolver::resolve_proc_header(Ast_Proc_Lit *proc_lit) {
    if (proc_lit->proc_resolve_state >= PROC_RESOLVE_STATE_HEADER) return;

    Scope *prev_scope = current_scope;
    proc_lit->scope = new_scope(global_scope, SCOPE_PROC);

    Auto_Array<Type*> params = {};
    for (Ast_Param *param : proc_lit->type->params) {
        Decl *param_decl = decl_variable_create(param->name->name);
        param_decl->type_expr = param->type;
        scope_add(current_scope, param_decl);
        resolve_decl(param_decl);
        params.push(param_decl->type);
    }
    Auto_Array<Type*> results = {};
    for (Ast *ret : proc_lit->type->results) {
        Type *ret_type = resolve_type(ret);
        results.push(ret_type);
    }
    Proc_Type *proc_type = proc_type_create(params, results);
    proc_lit->inferred_type = proc_type;

    current_scope = prev_scope;

    proc_lit->proc_resolve_state = PROC_RESOLVE_STATE_HEADER;
}

void Resolver::resolve_proc_body(Ast_Proc_Lit *proc_lit) {
    if (proc_lit->proc_resolve_state == PROC_RESOLVE_STATE_COMPLETE) return;

    Ast_Proc_Lit *prev_proc = current_proc;
    Scope *prev_scope = current_scope;

    current_proc = proc_lit;
    current_scope = proc_lit->scope;

    resolve_block(proc_lit->body);

    current_scope = prev_scope;
    current_proc = prev_proc;

    proc_lit->proc_resolve_state = PROC_RESOLVE_STATE_COMPLETE;
}

void Resolver::resolve_proc_lit(Ast_Proc_Lit *proc_lit) {
    if (proc_lit->proc_resolve_state == PROC_RESOLVE_STATE_COMPLETE) return;

    resolve_proc_header(proc_lit);
    resolve_proc_body(proc_lit);
}

void Resolver::resolve_proc_decl(Decl *decl) {
    Ast_Proc_Lit *proc_lit = (Ast_Proc_Lit *)decl->init_expr;
    resolve_proc_lit(proc_lit);
    decl->type = proc_lit->inferred_type;
}

void Resolver::resolve_decl(Decl *decl) {
    if (decl->resolve_state == RESOLVE_DONE) return;

    if (decl->resolve_state == RESOLVE_STARTED) {
        report_line("Cyclical declaration on '%s'.\n", decl->name->data);
        return;
    }

    Decl *prev_decl = current_decl;
    current_decl = decl;

    decl->resolve_state = RESOLVE_STARTED;

    switch (decl->kind) {
    case DECL_TYPE:
        resolve_type_decl(decl);
        break;
    case DECL_VARIABLE:
        resolve_variable_decl(decl);
        break;
    case DECL_CONSTANT:
        resolve_constant_decl(decl);
        break;
    case DECL_PROCEDURE:
        resolve_proc_decl(decl);
        break;
    }

    decl->resolve_state = RESOLVE_DONE;

    current_decl = prev_decl;
}

internal Select lookup_field(Type *type, Atom *name, bool is_type) {
    Select select = {};

    if (is_type) {
        if (type->is_struct_type()) {
            Struct_Type *st = (Struct_Type *)type;
            Decl *member = scope_find(st->scope, name);
            if (member && member->kind != DECL_VARIABLE) {
                select.decl = member;
                return select;
            }
        } else if (type->is_enum_type()) {
            Enum_Type *et = (Enum_Type *)type;
            Decl *field = scope_find(et->scope, name);
            select.decl = field;
            return select;
        }
    } else if (type->is_struct_type()) {
        Struct_Type *st = (Struct_Type *)type;
        Decl *member = scope_find(st->scope, name);
        if (member && member->kind == DECL_VARIABLE) {
            select.decl = member;
            return select;
        }
    } else if (type->is_enum_type()) {
    }

    return select;
}


Type *Resolver::resolve_struct_type(Ast_Struct_Type *type) {
    Decl *type_decl = nullptr;
    Atom *name = nullptr;

    if (current_decl->kind == DECL_TYPE) {
        name = current_decl->name;
        type_decl = current_decl;
    }

    if (type_decl) {
        type->scope = new_scope(current_scope, SCOPE_STRUCT);
    } else {
        type->scope = current_scope;
    }
    type->scope->name = name;

    Struct_Type *st = struct_type_create(name, {}, type->scope);

    for (Ast_Value_Decl *member : type->members) {
        resolve_value_decl_preamble(member);

        for (Ast *name : member->names) {
            Ast_Ident *ident = (Ast_Ident *)name;
            Decl *decl = ident->ref;
            Assert(decl);
            st->members.push(decl);
        }
    }

    for (Decl *member : st->members) {
        if (member->kind != DECL_PROCEDURE) {
            resolve_decl(member);
        }
    }

    if (type_decl) {
        exit_scope();
        type_decl->type_complete = true;
    }

    type->inferred_type = st;
    return st;
}

Type *Resolver::resolve_enum_type(Ast_Enum_Type *type) {
    type->scope = new_scope(current_scope, SCOPE_ENUM);

    Type *base_type = type_int;
    if (type->base_type) {
        base_type = resolve_type(type->base_type);
    }

    Constant_Value enumerant = {0};

    Enum_Type *et = enum_type_create(base_type, {}, type->scope);

    for (Ast_Enum_Field *field : type->fields) {
        Decl *found = scope_find(current_scope, field->name->name);

        if (found) {
            report_ast_error(field, "'%s' already defined in enumeration field.\n", field->name->name->data);
        }

        if (field->expr) {
            resolve_expr(field->expr);

            if (field->expr->mode == ADDRESSING_CONSTANT) {
                enumerant = field->expr->value;
            } else {
                report_ast_error(field->expr, "enumerant must be a constant value.\n");
            }
        }

        Decl *decl = decl_constant_create(field->name->name);
        decl->init_expr = field->expr;
        decl->type = et;
        decl->constant_value = enumerant;
        scope_add(current_scope, decl);
        et->fields.push(decl);
        bigint_add(&enumerant.value_integer, &enumerant.value_integer, 1);
    }

    exit_scope();

    return et;
}

Type *Resolver::resolve_type(Ast *type) {
    if (!type) return nullptr;

    //@Note add non-terminator types to type complete path.
    switch (type->kind) {
    case AST_IDENT: {
        Ast_Ident *ident = (Ast_Ident *)type;
        resolve_ident(ident);
        type_complete_path_clear();

        Decl *decl = ident->ref;
        if (decl && decl->kind == DECL_TYPE) {
            return decl->type;
        }
        break;
    }

    case AST_SELECTOR: {
        Ast_Selector *selector = (Ast_Selector *)type;

        resolve_selector_expr(selector);
        if (selector->mode == ADDRESSING_TYPE) {
            return selector->inferred_type;
        }
    }

    case AST_POINTER_TYPE: {
        Ast_Pointer_Type *pointer = (Ast_Pointer_Type *)type;
        type_complete_path_add(pointer);

        Type *elem = resolve_type(pointer->type);
        Pointer_Type *pt = pointer_type_create(elem);
        return pt;
    }

    case AST_ARRAY_TYPE: {
        Ast_Array_Type *array = (Ast_Array_Type *)type;
        type_complete_path_add(array);

        Type *elem = resolve_type(array->type);
        Array_Type *array_type = array_type_create(elem);
        if (array->length) {
            
        }
        // array_type->array_size = (u64)array->length->value.int_val;
        return array_type;
    }

    case AST_PROC_TYPE: {
        Ast_Proc_Type *proc = (Ast_Proc_Type *)type;

        Auto_Array<Type*> params = {};
        for (Ast_Param *param : proc->params) {
            Type *param_type = resolve_type(param->type);
            params.push(param_type);
        }

        Auto_Array<Type*> results = {};
        for (Ast *t : proc->results) {
            Type *ret = resolve_type(t);
            results.push(ret);
        }

        type_complete_path_clear();

        Proc_Type *pt = proc_type_create(params, results);
        return pt;
    }

    case AST_ENUM_TYPE: {
        Ast_Enum_Type *et = (Ast_Enum_Type *)type;
        Type *ty = resolve_enum_type(et);
        type_complete_path_clear();
        return ty;
    }

    case AST_STRUCT_TYPE: {
        Ast_Struct_Type *st = (Ast_Struct_Type *)type;
        Type *ts = resolve_struct_type(st);
        type_complete_path_clear();
        return ts;
    }
    }

    report_ast_error(type, "not a type.\n");
    return type_invalid;
}

void Resolver::resolve_value_decl(Ast_Value_Decl *vd, bool is_global) {
    if (is_global) {
        for (int i = 0; i < vd->names.count; i++) {
            Ast_Ident *name = (Ast_Ident *)vd->names[i];
            Decl *decl = name->ref;
            resolve_decl(decl);
        }
        return;
    }

    for (Ast *value : vd->values) {
        resolve_expr_base(value);
    }

    int total_values_count = get_values_count(vd->values);
    if (vd->names.count < total_values_count) {
        report_ast_error(vd, "mismatched number of values '%d' = '%d'.\n", vd->names.count, total_values_count);
    }

    if (vd->type) {
        Type *spec_type = resolve_type(vd->type);

        for (Ast *name : vd->names) {
            Ast_Ident *ident = (Ast_Ident *)name;
            Decl *decl = ident->ref;
            decl->type = spec_type;
        }

        for (Ast *value : vd->values) {
            if (!is_convertible(spec_type, value->inferred_type)) {
                report_ast_error(value, "cannot convert from '%s to '%s'.\n", string_from_type(value->inferred_type), string_from_type(spec_type));
            }
        }
    } else {
        if (total_values_count < vd->names.count) {
            for (int vidx = 0, nidx = 0; vidx < vd->values.count; vidx++) {
                Ast *value = vd->values[vidx];

                int type_count = type_value_count(value->inferred_type);

                for (int i = 0; i < type_count; i++) {
                    Type *type = type_from_index(value->inferred_type, i);
                    Ast_Ident *name = (Ast_Ident *)(vd->names[nidx + i]);
                    Decl *decl = name->ref;
                    decl->type = type;
                    nidx++;
                }
            }
        }
    }
}

void Resolver::resolve_global_value_decl(Ast_Value_Decl *vd) {
    resolve_value_decl(vd, true);
}

void Resolver::resolve_global_decl(Ast *decl) {
    switch (decl->kind) {
    case AST_VALUE_DECL: {
        Ast_Value_Decl *vd = (Ast_Value_Decl *)decl;
        resolve_global_value_decl(vd);
        break;
    }
    }
}

void Resolver::add_global_constant(String name, Type *type, Constant_Value value) {
    Decl *decl = decl_constant_create(atom_create(name));
    decl->type = type;
    decl->constant_value = value;
    decl->resolve_state = RESOLVE_DONE;
    scope_add(current_scope, decl);
}

//@Note Initializes Decl without resolving
void Resolver::resolve_value_decl_preamble(Ast_Value_Decl *vd) {
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
        }
    }
}

void Resolver::register_global_declarations() {
    Ast_Root *root = parser->root;

    global_scope = new_scope(current_scope, SCOPE_GLOBAL);
    root->scope = global_scope;

    add_global_constant(str_lit("true"), type_bool,  make_constant_value_int(bigint_make(0)));
    add_global_constant(str_lit("false"), type_bool, make_constant_value_int(bigint_make(1)));

    add_global_constant(str_lit("null"), type_null, make_constant_value_int(bigint_make(0)));

    for (Type *type : g_builtin_types) {
        if (type->name) {
            Decl *decl = decl_type_create(type->name);
            decl->type = type;
            decl->resolve_state = RESOLVE_DONE; //@Note Don't resolve for builtin
            type->decl = decl;
            scope_add(current_scope, decl);
        }
    }

    for (Ast *decl_node : root->decls) {
        switch (decl_node->kind) {
        case AST_VALUE_DECL: {
            Ast_Value_Decl *vd = (Ast_Value_Decl *)decl_node;
            vd->flags |= AST_FLAG_GLOBAL;

            if (vd->is_mutable) {
                for (int i = 0; i < vd->names.count; i++) {
                    Ast_Ident *name = (Ast_Ident *)vd->names[i];

                    Decl *found = scope_find(current_scope, name->name);
                    if (found) {
                        report_ast_error(name, "redeclaration of '%s'.\n", name->name->data);
                        continue;
                    }

                    Decl *decl = decl_variable_create(name->name);
                    decl->node = vd;
                    decl->type_expr = vd->type;
                    name->ref = decl;
                    scope_add(current_scope, decl);
                }

                if (vd->values.count > 0) {
                    if (vd->values.count != vd->names.count) {
                        report_ast_error(vd, "expected %d values, got %d values.\n", vd->values.count);
                        continue;
                    }

                    for (int i = 0; i < vd->values.count; i++) {
                        Ast_Ident *name = (Ast_Ident *)vd->names[i];
                        Ast *value = vd->values[i];
                        Decl *decl = name->ref;
                        decl->init_expr = value;
                    }
                }
            } else {
                if (vd->names.count != vd->values.count) {
                    report_ast_error(vd, "expected %d values, got %d values.\n", vd->values.count);
                    continue;
                }

                for (int i = 0; i < vd->names.count; i++) {
                    Ast_Ident *name = (Ast_Ident *)vd->names[i];
                    Ast *value = vd->values[i];

                    Decl *found = scope_find(current_scope, name->name);
                    if (found) {
                        report_ast_error(name, "redeclaration of '%s'.\n", name->name->data);
                        continue;
                    }

                    Decl *decl = nullptr;
                    if (is_ast_type(value)) {
                        decl = decl_type_create(name->name);
                    } else if (value->kind == AST_PROC_LIT) {
                        decl = decl_procedure_create(name->name);
                        decl->proc_lit = (Ast_Proc_Lit *)value;
                    } else {
                        decl = decl_constant_create(name->name);
                    }
                    decl->node = decl_node;
                    decl->type_expr = vd->type;
                    decl->init_expr = value;
                    name->ref = decl;
                    scope_add(current_scope, decl);
                }

                continue;
            }
        }
        }
    }
}

void Resolver::resolve() {
    Ast_Root *root = parser->root;

    register_global_declarations();

    for (Ast *decl : root->decls) {
        resolve_global_decl(decl);
    }
}
