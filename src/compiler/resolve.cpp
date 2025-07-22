#include <unordered_set>

void Resolver::type_complete_path_add(Ast *type) {
    array_add(&type_complete_path, type);
}

void Resolver::type_complete_path_clear() {
    array_clear(&type_complete_path);
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

internal int get_value_count(Type *type) {
    if (type->kind == TYPE_TUPLE) {
        Type_Tuple *tuple = (Type_Tuple *)type;
        return (int)tuple->types.count;
    }
    return 1;
}

internal int get_total_value_count(Array<Ast*> values) {
    int count = 0;
    for (Ast *v : values) {
        count += get_value_count(v->type);
    }
    return count;
}

internal Type *type_from_index(Type *type, int idx) {
    if (type->kind == TYPE_TUPLE) {
        Type_Tuple *tuple = (Type_Tuple *)type;
        Assert(idx < tuple->types.count);
        return tuple->types[idx];
    } else {
        Assert(idx == 0);
        return type;
    }
}

Resolver::Resolver(Parser *_parser) {
    arena = arena_create();
    parser = _parser;

    array_init(&type_complete_path, heap_allocator());
    array_init(&breakcont_stack, heap_allocator());
}

Scope *Resolver::new_scope(Scope *parent, Scope_Kind kind) {
    Scope *scope = scope_create(kind);
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
        Type_Struct *st = (Type_Struct *)type;
        for (Decl *member : st->members) {
            if (member->kind == DECL_PROCEDURE) {
                resolve_decl(member);
            }
        }
    }
}

void Resolver::resolve_variable_decl(Decl *decl) {
    if (decl->type_expr) {
        Type *type = resolve_type(decl->type_expr);
        decl->type = type;
    }
}

void Resolver::resolve_constant_decl(Decl *decl) {
    Ast *init = decl->init_expr;
    resolve_expr_base(init);

    decl->type = init->type;
    if (init->mode == ADDRESSING_CONSTANT) {
        decl->constant_value = init->value;
    } else if (init->mode == ADDRESSING_TYPE) {
        decl->constant_value = constant_value_typeid_make(init->type);
    } else {
        report_ast_error(init, "not a compile-time constant.\n");
    }
}


void Resolver::resolve_proc_header(Ast_Proc_Lit *proc_lit) {
    if (proc_lit->proc_resolve_state >= PROC_RESOLVE_STATE_HEADER) return;

    Scope *prev_scope = current_scope;
    proc_lit->scope = new_scope(global_scope, SCOPE_PROC);

    Ast_Proc_Type *proc_type = proc_lit->typespec;
    Type_Proc *pt = resolve_proc_type(proc_type, true);

    proc_lit->type = pt;

    current_scope = prev_scope;

    proc_lit->proc_resolve_state = PROC_RESOLVE_STATE_HEADER;
}

void Resolver::resolve_proc_body(Ast_Proc_Lit *proc_lit) {
    if (proc_lit->proc_resolve_state == PROC_RESOLVE_STATE_COMPLETE) return;

    Ast_Proc_Lit *prev_proc = current_proc;
    Scope *prev_scope = current_scope;

    for (Ast_Param *param : proc_lit->typespec->params) {
        array_add(&proc_lit->local_vars, (Ast *)param);
    }

    current_proc = proc_lit;
    current_scope = proc_lit->scope;

    if (proc_lit->body) {
        resolve_block(proc_lit->body);
    }

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
    decl->type = proc_lit->type;
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

internal Decl *lookup_field(Type *type, Atom *name, bool is_type) {
    Decl *select = nullptr;
    if (is_type) {
        if (is_struct_type(type)) {
            Type_Struct *ts = (Type_Struct *)type;
            Decl *member = scope_find(ts->scope, name);
            if (member && member->kind != DECL_VARIABLE) {
                select = member;
            }
        } else if (is_struct_type(type)) {
            Type_Union *tu = (Type_Union *)type;
            Decl *member = scope_find(tu->scope, name);
            if (member && member->kind != DECL_VARIABLE) {
                select = member;
            }
        } else if (is_enum_type(type)) {
            Type_Enum *et = (Type_Enum *)type;
            select = scope_find(et->scope, name);
        }
    } else if (is_struct_type(type) || is_union_type(type)) {
        Array<Decl*> members;
        if (is_struct_type(type)) {
            members = ((Type_Struct *)type)->members;
        } else if (is_union_type(type)) {
            members = ((Type_Union *)type)->members;
        }

        for (Decl *decl : members) {
            if (is_anonymous(decl)) {
                Decl *member = lookup_field(decl->type, name, is_type);
                if (member && member->kind == DECL_VARIABLE) {
                    select = member;
                }
            } else {
                if (atoms_match(decl->name, name)) {
                    if (decl->kind == DECL_VARIABLE) {
                        select = decl;
                    }
                }
            }
        }
    }
    return select;
}

Type_Proc *Resolver::resolve_proc_type(Ast_Proc_Type *proc_type, bool in_proc_lit) {
    int variadic_index = -1;

    auto params = array_make<Type*>(heap_allocator());
    for (int i = 0; i < proc_type->params.count; i++) {
        Ast_Param *param = proc_type->params[i];

        if (param->is_variadic) {
            variadic_index = i;
            // @Note Should be last for now...
            // Maybe allow parameters after variadic if default-value, or ensure callee specifies param field name.
            break;
        }

        Type *type = nullptr;
        if (in_proc_lit) {
            Decl *param_decl = decl_variable_create(param->name->name);
            param_decl->type_expr = param->typespec;
            scope_add(current_scope, param_decl);
            resolve_decl(param_decl);
            param->name->ref = param_decl;
            type = param_decl->type;
        } else {
            type = resolve_type(param->typespec);
        }

        param->type = type;
        array_add(&params, type);
    }

    auto results = array_make<Type*>(heap_allocator());
    for (Ast *ret : proc_type->results) {
        Type *ret_type = resolve_type(ret);
        array_add(&results, ret_type);
    }

    Type_Proc *pt = proc_type_create(params, results);
    pt->is_variadic = proc_type->is_variadic;
    pt->is_foreign  = proc_type->is_foreign;
    pt->variadic_index = variadic_index;
    pt->size = 8; //@Todo Type Sizes

    if (!in_proc_lit) {
        proc_type->mode = ADDRESSING_CONSTANT;
        proc_type->value = constant_value_typeid_make(pt);
    }

    return pt;
}

Type_Struct *Resolver::resolve_struct_type(Ast_Struct_Type *type) {
    Assert(current_decl);

    Atom *name = current_decl->name;

    type->scope = new_scope(current_scope, SCOPE_STRUCT);
    type->scope->name = name;

    Type_Struct *st = struct_type_create(name, array_make<Decl*>(heap_allocator()), type->scope);

    for (Ast_Value_Decl *member : type->members) {
        if (member->names.count > 0) {
            resolve_value_decl_preamble(member);
            for (Ast *name : member->names) {
                Ast_Ident *ident = (Ast_Ident *)name;
                Decl *decl = ident->ref;
                Assert(decl);
                array_add(&st->members, decl);
            }
        } else {
            //@Todo Cleanup anonymous members
            Ast *value = member->values[0];
            Assert(member->is_mutable);
            Decl *decl = nullptr;
            decl = decl_variable_create(nullptr);
            decl->node = member;
            decl->type_expr = value;
            scope_add(type->scope, decl);
            array_add(&st->members, decl);
        }
    }

    for (Decl *member : st->members) {
        if (member->kind != DECL_PROCEDURE) {
            resolve_decl(member);
        }
    }

    array_init(&st->offsets, heap_allocator());
    if (st->members.count > 0) {
        array_reserve(&st->offsets, st->members.count);
    }

    int struct_align = 0;
    int field_offset = 0;
    for (Decl *member : st->members) {
        if (member->kind == DECL_VARIABLE) {
            Type *field_type = member->type;
            int alignment = (int)field_type->size;
            if (alignment > struct_align) {
                struct_align = alignment;
            }
            field_offset = AlignForward(field_offset, alignment);
            array_add(&st->offsets, (s64)field_offset);
            field_offset += (int)field_type->size;
        }
    }
    st->size = AlignForward(field_offset, struct_align);

    // printf("STRUCT %s\n", st->name ? st->name->data : "");
    // for (s64 offset : st->offsets) {
    //     printf("offset: %lld\n", offset);
    // }
    // printf("---%lld\n", st->size);

    exit_scope();

    if (current_decl->kind == DECL_TYPE) current_decl->type_complete = true;

    type->type = st;
    return st;
}

Type_Union *Resolver::resolve_union_type(Ast_Union_Type *type) {
    Decl *type_decl = nullptr;
    Atom *name = nullptr;

    if (current_decl->kind == DECL_TYPE) {
        name = current_decl->name;
        type_decl = current_decl;
    }

    type->scope = current_scope;
    if (type_decl) {
        type->scope = new_scope(current_scope, SCOPE_UNION);
        type->scope->name = name;
        current_scope = type->scope;
    }

    Type_Union *ut = union_type_create(name, array_make<Decl*>(heap_allocator()), type->scope);

    for (Ast_Value_Decl *member : type->members) {
        if (member->names.count > 0) {
            resolve_value_decl_preamble(member);
            for (Ast *name : member->names) {
                Ast_Ident *ident = (Ast_Ident *)name;
                Decl *decl = ident->ref;
                Assert(decl);
                array_add(&ut->members, decl);
            }
        } else {
            //@Todo Cleanup anonymous members
            Ast *value = member->values[0];
            Assert(member->is_mutable);
            Decl *decl = nullptr;
            decl = decl_variable_create(nullptr);
            decl->node = member;
            decl->type_expr = value;
            scope_add(type->scope, decl);
            array_add(&ut->members, decl);
        }
    }

    for (Decl *member : ut->members) {
        if (member->kind != DECL_PROCEDURE) {
            resolve_decl(member);
        }
    }

    if (type_decl) {
        exit_scope();
        type_decl->type_complete = true;
    }

    type->type = ut;
    return ut;
}

Type_Enum *Resolver::resolve_enum_type(Ast_Enum_Type *type) {
    type->scope = new_scope(current_scope, SCOPE_ENUM);

    Type *base_type = type_int;
    if (type->base_type) {
        base_type = resolve_type(type->base_type);
    }

    bigint enumerant = bigint_make(0);

    Type_Enum *et = enum_type_create(base_type, {}, type->scope);
    array_init(&et->fields, heap_allocator());

    for (Ast_Enum_Field *field : type->fields) {
        Decl *found = scope_find(current_scope, field->name->name);

        if (found) {
            report_ast_error(field, "'%s' already defined in enumeration field.\n", field->name->name->data);
        }

        if (field->expr) {
            resolve_expr(field->expr);

            if (field->expr->mode == ADDRESSING_CONSTANT) {
                if (is_integral_type(field->expr->type)) {
                    enumerant = field->expr->value.value_integer;
                } else {
                    report_ast_error(field->expr, "enumerant must be of integral type.\n");
                }
            } else {
                report_ast_error(field->expr, "enumerant must be a constant value.\n");
            }
        }

        Decl *decl = decl_constant_create(field->name->name);
        decl->resolve_state = RESOLVE_DONE;
        decl->init_expr = field->expr;
        decl->type = et;
        decl->constant_value = constant_value_int_make(bigint_copy(&enumerant));
        scope_add(current_scope, decl);
        array_add(&et->fields, decl);
        bigint_add(&enumerant, &enumerant, 1);
    }

    et->size = base_type->size;

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
        if (decl) {
            if (decl->kind == DECL_TYPE) {
                return decl->type;
            } else if (decl->kind == DECL_CONSTANT) {
                if (decl->constant_value.kind == CONSTANT_VALUE_TYPEID) {
                    return decl->constant_value.value_typeid;
                }
            }
        }
        break;
    }

    case AST_SELECTOR: {
        Ast_Selector *selector = (Ast_Selector *)type;

        resolve_selector_expr(selector);
        if (selector->mode == ADDRESSING_TYPE) {
            return selector->type;
        }
    }

    case AST_POINTER_TYPE: {
        Ast_Pointer_Type *pointer = (Ast_Pointer_Type *)type;
        type_complete_path_add(pointer);

        Type *elem = resolve_type(pointer->elem);
        Type_Pointer *pt = pointer_type_create(elem);
        return pt;
    }

    case AST_ARRAY_TYPE: {
        Ast_Array_Type *array = (Ast_Array_Type *)type;
        type_complete_path_add(array);

        if (array->array_size) {
            resolve_expr(array->array_size);
            if (!is_integral_type(array->array_size->type)) {
                report_ast_error(array->array_size, "array size must be of integral type.\n");
            }
        }

        Type *elem = resolve_type(array->elem);

        if (array->array_size) {
            return array_type_create(elem, u64_from_bigint(array->array_size->value.value_integer));
        }
        if (array->is_view) {
            return array_view_type_create(elem);
        }
        Assert(array->is_dynamic);
        return dynamic_array_type_create(elem);
    }

    case AST_PROC_TYPE: {
        Ast_Proc_Type *proc = (Ast_Proc_Type *)type;
        Type_Proc *tp = resolve_proc_type(proc, false);
        type_complete_path_clear();
        return tp;
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

    case AST_UNION_TYPE: {
        Ast_Union_Type *ut = (Ast_Union_Type *)type;
        Type *tu = resolve_union_type(ut);
        type_complete_path_clear();
        return tu;
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

        //@Todo Check for simple value decls
        if (vd->is_mutable) {
            for (Ast *value : vd->values) {
                resolve_expr_base(value);
            }
        }

        return;
    }

    array_add(&current_proc->local_vars, (Ast *)vd);

    for (Ast *value : vd->values) {
        resolve_expr_base(value);
    }

    int total_values_count = get_total_value_count(vd->values);
    if (vd->is_mutable && total_values_count != 0) {
        if (vd->names.count != total_values_count) {
            report_ast_error(vd, "assignment mismatch: %d variables, %d values.\n", (int)vd->names.count, total_values_count);
        }
    }

    if (vd->typespec) {
        Type *spec_type = resolve_type(vd->typespec);

        for (Ast *name : vd->names) {
            Ast_Ident *ident = (Ast_Ident *)name;
            Decl *decl = ident->ref;
            decl->type = spec_type;
        }

        for (Ast *value : vd->values) {
            if (!is_convertible(spec_type, value->type)) {
                report_ast_error(value, "cannot convert from '%s to '%s'.\n", string_from_type(value->type), string_from_type(spec_type));
            }
        }
    } else {
        if (total_values_count <= vd->names.count) {
            for (int vidx = 0, nidx = 0; vidx < vd->values.count; vidx++) {
                Ast *value = vd->values[vidx];

                int type_count = get_value_count(value->type);

                for (int i = 0; i < type_count; i++) {
                    Type *type = type_from_index(value->type, i);
                    Ast_Ident *name = (Ast_Ident *)(vd->names[nidx]);
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
            decl->type_expr = vd->typespec;
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

    add_global_constant(str_lit("true"), type_bool,  constant_value_int_make(bigint_make(1)));
    add_global_constant(str_lit("false"), type_bool, constant_value_int_make(bigint_make(0)));

    add_global_constant(str_lit("null"), type_null, constant_value_int_make(bigint_make(0)));

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
                    decl->type_expr = vd->typespec;
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
                    decl->type_expr = vd->typespec;
                    decl->init_expr = value;
                    name->ref = decl;
                    scope_add(current_scope, decl);
                }
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
