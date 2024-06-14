#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"

extern Ast_Type_Info *t_uint;
extern Ast_Type_Info *t_int;
extern Ast_Type_Info *t_uint8;
extern Ast_Type_Info *t_uint16;
extern Ast_Type_Info *t_uint32;
extern Ast_Type_Info *t_uint64;
extern Ast_Type_Info *t_float32;
extern Ast_Type_Info *t_float64;
extern Ast_Type_Info *t_bool;
extern Ast_Type_Info *t_uint;
extern Ast_Type_Info *t_int;
extern Ast_Type_Info *t_bool;

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

    void register_global_declarations();
};

void init_builtin_types(Scope *scope);

inline void poison(Ast *node) {
    node->poisoned = true;
}

inline bool is_pointer_type(Ast_Type_Info *type) {
    bool result = type->kind == TypeKind_Pointer || type->kind == TypeKind_Array;
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

#endif // SEMANTIC_H
