#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"

struct Scope {
    Scope *parent = nullptr;
    Array<Ast_Declaration *> declarations;
};

struct Sema_Analyzer {
    Parser *parser;

    Scope *global_scope;
    Scope *current_scope;
    
    void error(Source_Loc loc, const char *fmt, ...);

    void enter_scope() {
        Scope *scope = new Scope();
        scope->parent = current_scope;
        current_scope = scope;
    }

    void exit_scope() {
        assert(current_scope->parent != nullptr);
        Scope *scope = current_scope->parent;
        delete current_scope;
        current_scope = scope; 
    }

    void init(Parser *_parser) {
        current_scope = global_scope = new Scope();
        parser = _parser;
    }

    void resolve();
    void resolve_expression(Ast_Expression *expression);
    void resolve_statement(Ast_Statement *statement);
    void resolve_declaration(Ast_Declaration *declaration);
    Ast_Type_Info *resolve_type_definition(Ast_Type_Definition *defn);

    void typecheck_arithmetic_expression(Ast_Binary_Expression *expression);

    void register_global_declarations();
};

void init_builtin_types(Scope *scope);

inline void poison(Ast *node) {
    node->poisoned = true;
}

inline bool is_pointer_type(Ast_Type_Info *type) {
    bool result = type->type_kind == TypeKind_Pointer || type->type_kind == TypeKind_Array;
    return result;
}

inline bool is_basic_type(Ast_Type_Info *type) {
    bool result = type->type_flags & TypeInfoFlag_Basic;
    return result;
}

inline bool is_integral_type(Ast_Type_Info *type) {
    bool result = type->type_flags & TypeInfoFlag_Integer;
    return result;
}

inline bool is_numeric_type(Ast_Type_Info *type) {
    bool result = type->type_flags & TypeInfoFlag_Integer || type->type_flags & TypeInfoFlag_Float;
    return result;
}

inline Ast_Type_Info *deref_type(Ast_Type_Info *type) {
    assert(type->type_kind == TypeKind_Pointer || type->type_kind == TypeKind_Array);
    return type->base;
}



#endif // SEMANTIC_H
