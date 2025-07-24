#if !defined(DECL_H)
#define DECL_H

#include "ast.h"

#define DECL_ALLOC(T) (alloc_item(heap_allocator(), T))
#define DECL_NEW(T) static_cast<T*>(&(*DECL_ALLOC(T) = T()))

struct BE_Proc;
struct BE_Var;

enum Decl_Kind {
    DECL_INVALID,
    DECL_TYPE,
    DECL_VARIABLE,
    DECL_CONSTANT,
    DECL_PROCEDURE,
};

struct Decl {
    Decl_Kind kind = DECL_INVALID;
    Resolve_State resolve_state = RESOLVE_UNSTARTED;

    Atom *name = nullptr;
    Type *type = nullptr;

    Ast *node = nullptr;

    Ast *type_expr = nullptr;
    Ast *init_expr = nullptr;

    Ast_Proc_Lit *proc_lit = nullptr;

    b32 type_complete = false;

    Constant_Value constant_value;

    union {
        BE_Var *backend_var = nullptr;
    };
};

enum Scope_Kind {
    SCOPE_GLOBAL,
    SCOPE_PROC,
    SCOPE_STRUCT,
    SCOPE_UNION,
    SCOPE_ENUM,
    SCOPE_BLOCK,
};

struct Scope {
    Scope_Kind kind;

    Scope *parent = nullptr;
    Scope *first = nullptr;
    Scope *last = nullptr;
    Scope *next = nullptr;
    Scope *prev = nullptr;

    Atom *name = nullptr;

    int level = 0;
    Array<Decl*> decls;
    Ast_Block *block;
};

bool is_anonymous(Decl *decl);

Decl *decl_type_create(Atom *name);
Decl *decl_variable_create(Atom *name);
Decl *decl_constant_create(Atom *name);
Decl *decl_procedure_create(Atom *name);

Scope *scope_create(Scope_Kind kind);
void scope_add(Scope *scope, Decl *decl);
Decl *scope_find(Scope *scope, Atom *name);
Decl *scope_lookup(Scope *scope, Atom *name);


#endif //DECL_H
