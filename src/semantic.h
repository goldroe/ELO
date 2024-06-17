#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"

struct Scope {
    Scope *parent = nullptr;
    Array<Ast_Declaration*> declarations;
};

struct Sema_Analyzer {
    Parser *parser;

    int error_count = 0;

    Scope *global_scope;
    Scope *current_scope;
    
    Array<Scope*> procedure_scope_stack;
    
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

    void enter_procedure() {
        procedure_scope_stack.push(current_scope);
        current_scope = global_scope;
        enter_scope();
    }

    void exit_procedure() {
        assert(current_scope != global_scope);
        exit_scope();
        if (!procedure_scope_stack.is_empty()) {
            current_scope = procedure_scope_stack.back();
            procedure_scope_stack.pop();
        }
    }

    void init(Parser *_parser) {
        current_scope = global_scope = new Scope();
        procedure_scope_stack.push(global_scope);
        parser = _parser;
    }

    void resolve();
    void resolve_block(Ast_Block *block);
    void resolve_expression(Ast_Expression *expression);
    void resolve_statement(Ast_Statement *statement);
    void resolve_declaration(Ast_Declaration *declaration);
    Ast_Type_Info *resolve_type_definition(Ast_Type_Definition *defn);

    void typecheck_arithmetic_expression(Ast_Binary_Expression *expression);
    void typecheck_boolean_expression(Ast_Binary_Expression *expression);
    void typecheck_comparison_expression(Ast_Binary_Expression *expression);

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

inline bool is_type(Ast_Type_Info *type, Type_Kind kind) {
    if (type) {
        return type->type_kind == kind;
    }
    return false;
}

inline Ast_Type_Info *deref_type(Ast_Type_Info *type) {
    assert(type->type_kind == TypeKind_Pointer || type->type_kind == TypeKind_Array);
    return type->base;
}

inline Ast_Type_Info *deref_field_pointer(Ast_Type_Info *type) {
    Ast_Type_Info *result = type;
    if (type->type_kind == TypeKind_Pointer) {
        result = deref_type(type);
    }
    return result;
}

#endif // SEMANTIC_H
