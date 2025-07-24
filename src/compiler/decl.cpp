#include "atom.h"
#include "decl.h"

bool is_anonymous(Decl *decl) {
    return decl->name == nullptr;
}

Decl *decl_type_create(Atom *name) {
    Decl *decl = DECL_NEW(Decl);
    decl->kind = DECL_TYPE;
    decl->name = name;
    return decl;
}

Decl *decl_variable_create(Atom *name) {
    Decl *decl = DECL_NEW(Decl);
    decl->kind = DECL_VARIABLE;
    decl->name = name;
    return decl;
}

Decl *decl_constant_create(Atom *name) {
    Decl *decl = DECL_NEW(Decl);
    decl->kind = DECL_CONSTANT;
    decl->name = name;
    return decl;
}

Decl *decl_procedure_create(Atom *name) {
    Decl *decl = DECL_NEW(Decl);
    decl->kind = DECL_PROCEDURE;
    decl->name = name;
    return decl;
}

Scope *scope_create(Scope_Kind kind) {
    Scope *scope = new Scope();
    scope->kind = kind;
    array_init(&scope->decls, heap_allocator());
    return scope;
}

void scope_add(Scope *scope, Decl *decl) {
    array_add(&scope->decls, decl);
}

Decl *scope_find(Scope *scope, Atom *name) {
    for (Decl *decl : scope->decls) {
        if (atoms_match(decl->name, name)) {
            return decl;
        }
    }
    return nullptr;
}

Decl *scope_lookup(Scope *scope, Atom *name) {
    for (; scope; scope = scope->parent) {
        for (Decl *decl : scope->decls) {
            if (atoms_match(decl->name, name)) {
                return decl;
            }
        }
    }
    return nullptr;
}
