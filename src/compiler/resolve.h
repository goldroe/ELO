#ifndef RESOLVE_H
#define RESOLVE_H

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

    Scope *parent = NULL;
    Scope *first = NULL;
    Scope *last = NULL;
    Scope *next = NULL;
    Scope *prev = NULL;

    int level = 0;
    Auto_Array<Ast_Decl*> declarations;
    // String_Map<Ast_Decl*> decl_map;
    Ast_Block *block;

    Ast_Decl *lookup(Atom *name);
};

struct Resolver {
    Arena *arena;

    Parser *parser;

    int error_count = 0;

    Scope *global_scope = NULL;
    Scope *current_scope = NULL;
    Ast_Proc *current_proc = NULL;

    Auto_Array<Ast*> breakcont_stack;

    Resolver(Parser *_parser);

    Type *resolve_type(Ast_Type_Defn *type_defn);

    bool in_global_scope();
    void add_entry(Ast_Decl *decl);
    Ast_Decl *lookup_local(Atom *name);
    Ast_Decl *lookup(Atom *name);
    Ast_Decl *lookup(Scope *scope, Atom *name);
    Scope *new_scope(Scope_Flags flags);
    void exit_scope();

    void resolve_block(Ast_Block *block);

    void resolve_control_path_flow(Ast_Proc *proc);
    void resolve_for_stmt(Ast_For *for_stmt);
    void resolve_while_stmt(Ast_While *while_stmt);
    void resolve_decl_stmt(Ast_Decl *decl);
    void resolve_if_stmt(Ast_If *if_stmt);
    void resolve_ifcase_stmt(Ast_Ifcase *ifcase);

    void resolve_return_stmt(Ast_Return *return_stmt);
    void resolve_break_stmt(Ast_Break *break_stmt);
    void resolve_continue_stmt(Ast_Continue *continue_stmt);
    void resolve_fallthrough_stmt(Ast_Fallthrough *fallthrough);

    void resolve_stmt(Ast_Stmt *stmt);

    Ast_Decl *lookup_overloaded(Atom *name, Auto_Array<Ast_Expr*> arguments, bool *overloaded);
    Ast_Operator_Proc *lookup_user_defined_binary_operator(OP op, Type *lhs, Type *rhs);
    Ast_Operator_Proc *lookup_user_defined_unary_operator(OP op, Type *type);

    void resolve_user_defined_binary_expr(Ast_Binary *expr);
    void resolve_builtin_binary_expr(Ast_Binary *expr);
    void resolve_binary_expr(Ast_Binary *expr);

    void resolve_user_defined_unary_expr(Ast_Unary *expr);
    void resolve_builtin_unary_expr(Ast_Unary *expr);
    void resolve_unary_expr(Ast_Unary *unary);

    void resolve_assignment_expr(Ast_Assignment *assignment);

    void resolve_ident(Ast_Ident *ident);
    void resolve_range_expr(Ast_Range *range);
    void resolve_access_expr(Ast_Access *access);
    void resolve_address_expr(Ast_Address *address);
    void resolve_deref_expr(Ast_Deref *deref);
    void resolve_call_expr(Ast_Call *call);
    void resolve_literal(Ast_Literal *literal);
    void resolve_cast_expr(Ast_Cast *cast);
    void resolve_subscript_expr(Ast_Subscript *subscript);
    void resolve_compound_literal(Ast_Compound_Literal *literal);
    void resolve_expr(Ast_Expr *expr);

    void resolve_proc_header(Ast_Proc *proc);
    void resolve_proc(Ast_Proc *proc);
    void resolve_struct(Ast_Struct *struct_decl);
    void resolve_enum(Ast_Enum *enum_decl);
    void resolve_var(Ast_Var *var);
    void resolve_param(Ast_Param *param);
    void resolve_decl(Ast_Decl *decl);
    void resolve_overloaded_proc(Ast_Proc *proc);
    void resolve_type_decl(Ast_Type_Decl *type_decl);

    void register_global_declarations();
    void resolve();

    Eval eval_unary_expr(Ast_Unary *u);
    Eval eval_binary_expr(Ast_Binary *b);
};

#endif // RESOLVE_H
