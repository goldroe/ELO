#if !defined(DECL_H)
#define DECL_H

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

enum Scope_Flags {
    SCOPE_GLOBAL  = (1<<0),
    SCOPE_PROC    = (1<<1),
    SCOPE_BLOCK   = (1<<2),
    SCOPE_STRUCT  = (1<<3),
    SCOPE_ENUM    = (1<<4),
};
EnumDefineFlagOperators(Scope_Flags);

struct Scope {
    Scope_Flags scope_flags;

    Scope *parent = nullptr;
    Scope *first = nullptr;
    Scope *last = nullptr;
    Scope *next = nullptr;
    Scope *prev = nullptr;

    Atom *name = nullptr;

    int level = 0;
    Auto_Array<Decl*> decls;
    Ast_Block *block;

    Ast_Decl *lookup(Atom *name);
};


#endif //DECL_H
