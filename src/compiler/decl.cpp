internal Decl *decl_type_create(Atom *name) {
    Decl *decl = DECL_NEW(Decl);
    decl->kind = DECL_TYPE;
    decl->name = name;
    return decl;
}

internal Decl *decl_variable_create(Atom *name) {
    Decl *decl = DECL_NEW(Decl);
    decl->kind = DECL_VARIABLE;
    decl->name = name;
    return decl;
}

internal Decl *decl_constant_create(Atom *name) {
    Decl *decl = DECL_NEW(Decl);
    decl->kind = DECL_CONSTANT;
    decl->name = name;
    return decl;
}

internal Decl *decl_procedure_create(Atom *name) {
    Decl *decl = DECL_NEW(Decl);
    decl->kind = DECL_PROCEDURE;
    decl->name = name;
    return decl;
}

internal Scope *make_scope(Scope_Flags flags) {
    Scope *result = new Scope();
    result->scope_flags = flags;
    return result;
}

internal void scope_add(Scope *scope, Decl *decl) {
    scope->decls.push(decl);
}

internal Decl *scope_find(Scope *scope, Atom *name) {
    for (Decl *decl : scope->decls) {
        if (atoms_match(decl->name, name)) {
            return decl;
        }
    }
    return nullptr;
}

internal Decl *scope_lookup(Scope *scope, Atom *name) {
    for (; scope; scope = scope->parent) {
        for (Decl *decl : scope->decls) {
            if (atoms_match(decl->name, name)) {
                return decl;
            }
        }
    }
    return nullptr;
}

