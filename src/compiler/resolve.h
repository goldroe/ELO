#ifndef RESOLVE_H
#define RESOLVE_H

struct Select {
    Decl *decl = nullptr;
    int index = 0;
};

struct Resolver {
    Arena *arena;

    Parser *parser;

    int error_count = 0;

    Scope *global_scope = nullptr;
    Scope *current_scope = nullptr;

    Decl *current_decl = nullptr;

    Ast_Proc_Lit *current_proc = nullptr;

    Array<Ast*> type_complete_path;

    Array<Ast*> breakcont_stack;

    Resolver(Parser *_parser);

    void Resolver::type_complete_path_add(Ast *type);
    void Resolver::type_complete_path_clear();


    void Resolver::resolve_expr_base(Ast *expr);
    void Resolver::resolve_single_value(Ast *expr);

    void resolve_block(Ast_Block *block);

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

    void Resolver::resolve_stmt(Ast *stmt);

    Ast_Decl *lookup_overloaded(Atom *name, Array<Ast*> arguments, bool *overloaded);

    void resolve_user_defined_binary_expr(Ast_Binary *expr);
    void resolve_builtin_binary_expr(Ast_Binary *expr);
    void resolve_binary_expr(Ast_Binary *expr);

    void resolve_user_defined_unary_expr(Ast_Unary *expr);
    void resolve_builtin_unary_expr(Ast_Unary *expr);
    void resolve_unary_expr(Ast_Unary *unary);

    void Resolver::resolve_assignment_stmt(Ast_Assignment *assign);

    void resolve_ident(Ast_Ident *ident);
    void resolve_range_expr(Ast_Range *range);
    void resolve_selector_expr(Ast_Selector *selector);
    void resolve_address_expr(Ast_Address *address);
    void resolve_deref_expr(Ast_Deref *deref);
    void resolve_call_expr(Ast_Call *call);
    void resolve_literal(Ast_Literal *literal);
    void resolve_cast_expr(Ast_Cast *cast);
    void resolve_subscript_expr(Ast_Subscript *subscript);
    void resolve_compound_literal(Ast_Compound_Literal *literal);
    void resolve_expr(Ast *expr);

    // void resolve_proc_header(Ast_Proc *proc);
    // void resolve_proc(Ast_Proc *proc);
    // void resolve_struct(Ast_Struct *struct_decl);
    // void resolve_enum(Ast_Enum *enum_decl);
    // void resolve_var(Ast_Var *var);
    // void resolve_param(Ast_Param *param);
    // void resolve_decl(Ast_Decl *decl);
    // void resolve_overloaded_proc(Ast_Proc *proc);
    // void resolve_type_decl(Ast_Type_Decl *type_decl);

    Constant_Value eval_unary_expr(Ast_Unary *u);
    Constant_Value eval_binary_expr(Ast_Binary *b);

    Scope *new_scope(Scope *parent, Scope_Flags flags);
    void exit_scope();

    bool in_global_scope();
    void register_global_declarations();

    Type *resolve_type(Ast *type);
    
    void Resolver::add_global_constant(String name, Type *type, Constant_Value value);

    void Resolver::resolve_proc_header(Ast_Proc_Lit *proc_lit);
    void Resolver::resolve_proc_body(Ast_Proc_Lit *proc_lit);
    void Resolver::resolve_proc_lit(Ast_Proc_Lit *proc_lit);

    void Resolver::resolve_type_decl(Decl *decl);
    void Resolver::resolve_proc_decl(Decl *decl);
    void Resolver::resolve_decl(Decl *decl);
    void Resolver:: resolve_variable_decl(Decl *decl);
    void Resolver:: resolve_constant_decl(Decl *decl);
    void Resolver::resolve_global_decl(Ast *decl);

    void Resolver::resolve_value_decl_preamble(Ast_Value_Decl *vd);
    void Resolver::resolve_value_decl(Ast_Value_Decl *vd, bool is_global);
    void Resolver::resolve_value_decl_stmt(Ast_Value_Decl *vd);
    void Resolver::resolve_global_value_decl(Ast_Value_Decl *vd);

    Type *Resolver::resolve_enum_type(Ast_Enum_Type *type);
    Type *Resolver::resolve_struct_type(Ast_Struct_Type *type);

    void resolve();
};


#endif // RESOLVE_H
