#include "parser.h"
#include "semantic.h"
#include <stdio.h>

Ast_Type_Info *t_uint;
Ast_Type_Info *t_int;
Ast_Type_Info *t_uint8;
Ast_Type_Info *t_uint16;
Ast_Type_Info *t_uint32;
Ast_Type_Info *t_uint64;
Ast_Type_Info *t_int8;
Ast_Type_Info *t_int16;
Ast_Type_Info *t_int32;
Ast_Type_Info *t_int64;
Ast_Type_Info *t_float32;
Ast_Type_Info *t_float64;
Ast_Type_Info *t_bool;

Ast_Declaration *make_builtin_type(Scope *scope, Atom *name, Ast_Type_Info *bt) {
    Ast_Ident *ident = make_ident(name);
    Ast_Declaration *declaration = make_declaration(ident);
    declaration->inferred_type = bt;
    declaration->resolved = true;
    scope->declarations.push(declaration);
    return declaration;
}

void init_builtin_types(Scope *scope) {
    t_uint    = make_type_info(TYPE_UINT, TYPE_BASIC | TYPE_INTEGER, 32);
    t_int     = make_type_info(TYPE_INT, TYPE_BASIC | TYPE_INTEGER | TYPE_SIGNED, 32);
    t_uint8   = make_type_info(TYPE_UINT8, TYPE_BASIC | TYPE_INTEGER, 8);
    t_uint16  = make_type_info(TYPE_UINT16, TYPE_BASIC | TYPE_INTEGER, 16);
    t_uint32  = make_type_info(TYPE_UINT32, TYPE_BASIC | TYPE_INTEGER, 32);
    t_uint64  = make_type_info(TYPE_UINT64, TYPE_BASIC | TYPE_INTEGER, 64);
    t_int8    = make_type_info(TYPE_INT8, TYPE_BASIC | TYPE_INTEGER | TYPE_SIGNED, 8);
    t_int16   = make_type_info(TYPE_INT16, TYPE_BASIC | TYPE_INTEGER | TYPE_SIGNED, 16);
    t_int32   = make_type_info(TYPE_INT32, TYPE_BASIC | TYPE_INTEGER | TYPE_SIGNED, 32);
    t_int64   = make_type_info(TYPE_INT64, TYPE_BASIC | TYPE_INTEGER | TYPE_SIGNED, 64);
    t_float32 = make_type_info(TYPE_FLOAT32, TYPE_BASIC | TYPE_FLOAT | TYPE_SIGNED, 32);
    t_float32 = make_type_info(TYPE_FLOAT64, TYPE_BASIC | TYPE_FLOAT | TYPE_SIGNED, 64);
    t_bool    = make_type_info(TYPE_BOOL, TYPE_BASIC | TYPE_INTEGER, 32);

    make_builtin_type(scope, make_atom("uint"), t_uint);
    make_builtin_type(scope, make_atom("int"), t_int);
    make_builtin_type(scope, make_atom("uint8"), t_uint8);
    make_builtin_type(scope, make_atom("uint16"), t_uint16);
    make_builtin_type(scope, make_atom("uint32"), t_uint32);
    make_builtin_type(scope, make_atom("uint64"), t_uint64);
    make_builtin_type(scope, make_atom("int8"),  t_int8);
    make_builtin_type(scope, make_atom("int16"), t_int16);
    make_builtin_type(scope, make_atom("int32"), t_int32);
    make_builtin_type(scope, make_atom("int64"), t_int64);
    make_builtin_type(scope, make_atom("float32"), t_float32);
    make_builtin_type(scope, make_atom("float64"), t_float64);
    make_builtin_type(scope, make_atom("bool"), t_bool);
}

Ast_Declaration *scope_lookup(Scope *local_scope, Atom *name) {
    for (Scope *scope = local_scope; scope; scope = scope->parent) {
        for (int i = 0; i < scope->declarations.count; i++) {
            Ast_Declaration *declaration = scope->declarations[i];
            if (atoms_match(name, declaration->ident->name)) {
                return declaration;
            }
        }
    }
    return nullptr;
}

Ast_Declaration *local_lookup(Scope *local_scope, Atom *name) {
    for (int i = 0; i < local_scope->declarations.count; i++) {
        Ast_Declaration *declaration = local_scope->declarations[i];
        if (atoms_match(name, declaration->ident->name)) {
            return declaration;
        }
    }
    return nullptr;
}

void Sema_Analyzer::register_global_declarations() {
    for (int i = 0; i < parser->root->declarations.count; i++) {
        Ast_Declaration *declaration = parser->root->declarations[i];

        Ast_Declaration *lookup = scope_lookup(global_scope, declaration->ident->name);
        if (lookup) {
            printf("'%s' already declared\n", lookup->ident->name->name);
        } else {
            global_scope->declarations.push(declaration);
        }
    }
}

void scope_add_declaration(Scope *scope, Ast_Declaration *declaration) {
    scope->declarations.push(declaration);
}

void Sema_Analyzer::resolve() {
    init_builtin_types(global_scope);
    register_global_declarations();

    for (int i = 0; i < parser->root->declarations.count; i++) {
        Ast_Declaration *declaration = parser->root->declarations[i];
        resolve_declaration(declaration);
    }
}

Ast_Type_Info *Sema_Analyzer::resolve_type_definition(Ast_Type_Definition *defn) {
    if (!defn) return nullptr;
    
    Ast_Type_Info *type_info = nullptr;
    for (Ast_Type_Definition *it = defn; it; it = it->base) {
        if (it->defn_flags & TYPE_DEFN_POINTER)
            type_info = make_type_info(TYPE_POINTER, 0, 64, type_info);
        else if (it->defn_flags & TYPE_DEFN_ARRAY)
            type_info = make_type_info(TYPE_ARRAY, 0, 64, type_info);
        else if (it->defn_flags & TYPE_DEFN_IDENT) {
            Ast_Declaration *decl = scope_lookup(current_scope, it->ident->name);
            if (decl) {
                resolve_declaration(decl);
                if (type_info) type_info->base = decl->inferred_type;
                else type_info = decl->inferred_type;
            } else {
                printf("undeclared identifier '%s'\n", it->ident->name->name);
            }
        }
    }
    return type_info;
}

void Sema_Analyzer::resolve_expression(Ast_Expression *expression) {
    if (!expression) return;
    switch (expression->type) {
    case AST_IDENT:
    {
        Ast_Ident *ident = static_cast<Ast_Ident *>(expression);
        Ast_Declaration *lookup = scope_lookup(current_scope, ident->name);
        if (!lookup) {
            printf("undeclared identifier '%s'\n", ident->name->name);
        }
        resolve_declaration(lookup);
        ident->inferred_type = lookup->inferred_type;
        break;
    }
    case AST_LITERAL:
    {
        Ast_Literal *literal = static_cast<Ast_Literal *>(expression);
        Ast_Type_Info *type_info = nullptr;
        if (literal->literal_flags & LITERAL_NUMBER) {
            if (literal->literal_flags & LITERAL_FLOAT) {
                type_info = t_float32;
            } else {
                type_info = t_int32; 
            }
        }
        // else if (literal->literal_flags & LITERAL_STRING) {
            // type_info = t_string;
        // }
        literal->inferred_type = type_info;
        break;
    }
    case AST_UNARY_EXPRESSION:
    {
        Ast_Unary_Expression *unary = static_cast<Ast_Unary_Expression *>(expression);
        resolve_expression(unary->expression);
        break;
    }
    case AST_BINARY_EXPRESSION:
    {
        Ast_Binary_Expression *binary = static_cast<Ast_Binary_Expression *>(expression);
        resolve_expression(binary->lhs);
        resolve_expression(binary->rhs);

        bool convertible = true;
        Ast_Type_Info *lhs = binary->lhs->inferred_type;
        Ast_Type_Info *rhs = binary->rhs->inferred_type;
        while (lhs || rhs) {
            if (lhs && rhs) {
                // eg. int = *int
                if (is_pointer_type(lhs) && !is_pointer_type(rhs) ||
                    !is_pointer_type(lhs) && is_pointer_type(rhs)) {
                    convertible = false;
                    break;
                }
                // eg. int = struct
                if (is_basic_type(lhs) && !is_basic_type(rhs) ||
                    !is_basic_type(lhs) && is_basic_type(rhs)) {
                    convertible = false;
                    break;
                }
            } else {
                // @note different levels of indirection
                convertible = false;
                break;
            }
            lhs = lhs->base;
            rhs = rhs->base;
        }

        if (!convertible) {
            printf("cannot convert from '%s' ", type_to_string(binary->rhs->inferred_type));
            printf("to '%s'\n", type_to_string(binary->lhs->inferred_type));
        }

        if (is_assign_operator(binary->op)) {
            if (!(binary->lhs->expr_flags & EXPR_LVALUE)) {
                printf("left operand must be l-value\n");
            }

            // @note if lhs is lvalue then whole expression is lvalue
            if (binary->lhs->expr_flags & EXPR_LVALUE) {
                binary->expr_flags |= EXPR_LVALUE;
            }
            
        }

        if (convertible) {
            binary->inferred_type = binary->lhs->inferred_type;
        }
        break; 
    }
    }
}

void Sema_Analyzer::resolve_statement(Ast_Statement *statement) {
    if (!statement) return;

    switch (statement->type) {
    case AST_BLOCK_STATEMENT:
    {
        Ast_Block_Statement *block_statement = static_cast<Ast_Block_Statement *>(statement);
        Ast_Block *block = block_statement->block;
        enter_scope();
        for (int i = 0; i < block->statements.count; i++) {
            Ast_Statement *stmt = block->statements[i];
            resolve_statement(stmt);
        }
        exit_scope();
        break;
    }
    case AST_DECLARATION_STATEMENT:
    {
        Ast_Declaration_Statement *decl_statement = static_cast<Ast_Declaration_Statement *>(statement);
        resolve_declaration(decl_statement->declaration);
        break;
    }
    case AST_EXPRESSION_STATEMENT:
    {
        Ast_Expression_Statement *expression_statement = static_cast<Ast_Expression_Statement *>(statement);
        resolve_expression(expression_statement->expression);
        break;
    }
    }
}

void Sema_Analyzer::resolve_declaration(Ast_Declaration *declaration) {
    if (!declaration) return;
    if (declaration->resolved) return;
    if (declaration->resolving) {
        assert(false && "Cyclical resolution");
        return;
    }
    declaration->resolving = true;

    // @note Name resolution of local declarations.
    // Does not check global declarations because they already get checked in register_global_declarations.
    if (!(declaration->declaration_flags & DECLARATION_GLOBAL)) {
        Ast_Declaration *lookup = scope_lookup(current_scope, declaration->ident->name);
        if (lookup) {
            printf("'%s' already declared\n", lookup->ident->name->name);
        } else {
            scope_add_declaration(current_scope, declaration);
        }
    }

    switch (declaration->type) {
    case AST_STRUCT:
    {
        Ast_Struct_Declaration *struct_declaration = static_cast<Ast_Struct_Declaration *>(declaration);
        for (int i = 0; i < struct_declaration->fields.count; i++) {
            Ast_Struct_Field *field = struct_declaration->fields[i];
            Ast_Type_Info *type_info = resolve_type_definition(field->type_definition);
            field->inferred_type = type_info;
        }
        break;
    }
    case AST_PROCEDURE:
    {
        Ast_Procedure_Declaration *procedure = static_cast<Ast_Procedure_Declaration *>(declaration);
        Ast_Type_Info *type_info = resolve_type_definition(procedure->return_type);
        procedure->inferred_type = type_info;

        enter_scope();
        for (int i = 0; i < procedure->parameters.count; i++) {
            Ast_Variable *param = procedure->parameters[i];
            resolve_declaration(param);
        }

        for (int i = 0; i < procedure->body->statements.count; i++) {
            Ast_Statement *statement = procedure->body->statements[i];
            resolve_statement(statement);
        }
        exit_scope();
        break;
    }
    case AST_VARIABLE:
    {
        Ast_Variable *variable = static_cast<Ast_Variable *>(declaration);
        Ast_Type_Info *type_info = resolve_type_definition(variable->type_definition);
        // @todo check specified type and intiializer are compatible
        resolve_expression(variable->initializer);
        variable->inferred_type = type_info;
        break;
    }
    }

    declaration->resolving = false;
    declaration->resolved = true;
}
