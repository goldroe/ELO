#ifndef RESOLVE_H
#define RESOLVE_H

struct Ast_Expr_Node {
    Ast_Expr_Node *next;
    Ast_Expr *expr;
};

struct Ast_Expr_Stack {
    Ast_Expr_Node *top;
    Ast_Expr_Node *free;
};

struct Resolver {
    Arena *arena;

    Parser *parser;

    int error_count = 0;

    Ast_Scope *global_scope = NULL;
    Ast_Scope *current_scope = NULL;
    Ast_Proc *current_proc = NULL;

    Resolver(Parser *_parser);

    void error(Ast *elem, const char *fmt, ...);

    Ast_Type_Info *resolve_type(Ast_Type_Defn *type_defn);

    bool in_global_scope();
    void add_entry(Ast_Decl *decl);
    Ast_Decl *lookup_local(Atom *name);
    Ast_Decl *lookup(Atom *name);
    Ast_Decl *lookup(Ast_Scope *scope, Atom *name);
    Ast_Scope *new_scope(Scope_Flags flags);
    void exit_scope();

    void resolve_block(Ast_Block *block);

    void resolve_control_path_flow(Ast_Proc *proc);
    void resolve_while_stmt(Ast_While *while_stmt);
    void resolve_decl_stmt(Ast_Decl_Stmt *decl_stmt);
    void resolve_if_stmt(Ast_If *if_stmt);
    void resolve_return_stmt(Ast_Return *return_stmt);
    void resolve_stmt(Ast_Stmt *stmt);

    void resolve_ident(Ast_Ident *ident);
    void resolve_range_expr(Ast_Range *range);
    void resolve_field_expr(Ast_Field *field);
    void resolve_address_expr(Ast_Address *address);
    void resolve_deref_expr(Ast_Deref *deref);
    void resolve_call_expr(Ast_Call *call);
    void resolve_literal(Ast_Literal *literal);
    void resolve_binary_expr(Ast_Binary *binary);
    void resolve_cast_expr(Ast_Cast *cast);
    void resolve_index_expr(Ast_Index *index);
    void resolve_compound_literal(Ast_Compound_Literal *literal);
    void resolve_unary_expr(Ast_Unary *unary);
    void resolve_expr(Ast_Expr *expr);

    void resolve_proc_header(Ast_Proc *proc);
    void resolve_proc(Ast_Proc *proc);
    void resolve_struct(Ast_Struct *struct_decl);
    void resolve_enum(Ast_Enum *enum_decl);
    void resolve_var(Ast_Var *var);
    void resolve_param(Ast_Param *param);
    void resolve_decl(Ast_Decl *decl);

    void register_global_declarations();
    void resolve();
};

#endif // RESOLVE_H
