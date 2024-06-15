#include "parser.h"
#include "semantic.h"
#include <stdio.h>
#include <stdarg.h>

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
Ast_Type_Info *t_string;

void Sema_Analyzer::error(Source_Loc loc, const char *fmt, ...) {
    printf("%s(%d,%d) error: ", parser->lexer->source_name, loc.line, loc.column);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

Ast_Declaration *make_builtin_type(Scope *scope, Atom *name, Ast_Type_Info *bt) {
    Ast_Ident *ident = make_ident(name);
    Ast_Declaration *declaration = make_declaration(ident);
    declaration->inferred_type = bt;
    declaration->resolved = true;
    scope->declarations.push(declaration);
    return declaration;
}

void init_builtin_types(Scope *scope) {
    t_uint    = make_type_info(TypeKind_Uint,    TypeInfoFlag_Basic|TypeInfoFlag_Integer, 32);
    t_uint8   = make_type_info(TypeKind_Uint8,   TypeInfoFlag_Basic|TypeInfoFlag_Integer, 8);
    t_uint16  = make_type_info(TypeKind_Uint16,  TypeInfoFlag_Basic|TypeInfoFlag_Integer, 16);
    t_uint32  = make_type_info(TypeKind_Uint32,  TypeInfoFlag_Basic|TypeInfoFlag_Integer, 32);
    t_uint64  = make_type_info(TypeKind_Uint64,  TypeInfoFlag_Basic|TypeInfoFlag_Integer, 64);
    t_int     = make_type_info(TypeKind_Int,     TypeInfoFlag_Basic|TypeInfoFlag_Integer|TypeInfoFlag_Signed, 32);
    t_int8    = make_type_info(TypeKind_Int8,    TypeInfoFlag_Basic|TypeInfoFlag_Integer|TypeInfoFlag_Signed, 8);
    t_int16   = make_type_info(TypeKind_Int16,   TypeInfoFlag_Basic|TypeInfoFlag_Integer|TypeInfoFlag_Signed, 16);
    t_int32   = make_type_info(TypeKind_Int32,   TypeInfoFlag_Basic|TypeInfoFlag_Integer|TypeInfoFlag_Signed, 32);
    t_int64   = make_type_info(TypeKind_Int64,   TypeInfoFlag_Basic|TypeInfoFlag_Integer|TypeInfoFlag_Signed, 64);
    t_float32 = make_type_info(TypeKind_Float32, TypeInfoFlag_Basic|TypeInfoFlag_Float  |TypeInfoFlag_Signed, 32);
    t_float64 = make_type_info(TypeKind_Float64, TypeInfoFlag_Basic|TypeInfoFlag_Float  |TypeInfoFlag_Signed, 64);
    t_bool    = make_type_info(TypeKind_Bool,    TypeInfoFlag_Basic|TypeInfoFlag_Integer, 32);
    t_string  = make_type_info(TypeKind_String,  TypeInfoFlag_Nil, 0);

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
    make_builtin_type(scope, make_atom("string"), t_string);
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
            error(declaration->ident->start, "'%s' already declared", lookup->ident->name->name);
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
    Ast_Type_Info *type_info = nullptr;
    for (Ast_Type_Definition *it = defn; it; it = it->base) {
        if (it->defn_flags & TypeDefnFlag_Pointer)
            type_info = make_type_info(TypeKind_Pointer, TypeInfoFlag_Nil, 64, type_info);
        else if (it->defn_flags & TypeDefnFlag_Array)
            type_info = make_type_info(TypeKind_Array, TypeInfoFlag_Nil, 64, type_info);
        else if (it->defn_flags & TypeDefnFlag_Ident) {
            Ast_Declaration *decl = scope_lookup(current_scope, it->ident->name);
            if (decl) {
                resolve_declaration(decl);
                if (type_info) type_info->base = decl->inferred_type;
                else type_info = decl->inferred_type;
            } else {
                error(it->ident->start, "undeclared identifier '%s'\n", it->ident->name->name);
            }
        }
    }
    return type_info;
}

Ast_Type_Info *get_bigger_type(Ast_Type_Info *first, Ast_Type_Info *last) {
    Ast_Type_Info *result;
    if (first->bits >= last->bits) {
        result = first;
    } else {
        result = last;
    }
    return result;
}

void Sema_Analyzer::typecheck_arithmetic_expression(Ast_Binary_Expression *expression) {
    Token_Type op = expression->op;
    Ast_Expression *lhs = expression->lhs;
    Ast_Expression *rhs = expression->rhs;
    Ast_Type_Info *result = lhs->inferred_type;

    switch (op) {
    case Token_Plus:
    case Token_Minus:
    case Token_Star:
    case Token_Slash:
    case Token_Amper:
    case Token_Bar:
    case Token_Xor:
    {
        if (lhs->inferred_type->type_kind == TypeKind_Struct || lhs->inferred_type->type_kind == TypeKind_Procedure) {
            char *type_s = type_to_string(lhs->inferred_type);
            error(expression->start, "invalid '%s', left operand is '%s'", token_type_to_string(op), type_s);
            free(type_s);
        }
        if (rhs->inferred_type->type_kind == TypeKind_Struct || rhs->inferred_type->type_kind == TypeKind_Procedure) {
            char *type_s = type_to_string(rhs->inferred_type);
            error(expression->start, "invalid '%s', right operand is '%s'", token_type_to_string(op), type_s);
            free(type_s);
        }
        break;
    }

    case Token_Percent:
    {
        bool lhs_valid = is_integral_type(lhs->inferred_type);
        bool rhs_valid = is_integral_type(rhs->inferred_type);
        if (!lhs_valid) {
            char *type = type_to_string(lhs->inferred_type);
            error(expression->lhs->start, "invalid '%', left operand is '%s'", type);
            free(type);
        }
        if (!rhs_valid) {
            char *type = type_to_string(rhs->inferred_type);
            error(expression->start, "invalid '%', right operand is '%s'", type);
            free(type);
        }
        break;
    }
    }

    Ast_Type_Info *left_type = lhs->inferred_type;
    Ast_Type_Info *right_type = rhs->inferred_type;
    if (left_type->type_flags & TypeInfoFlag_Integer && right_type->type_flags & TypeInfoFlag_Integer) {
        result = get_bigger_type(left_type, right_type);
    } else if (left_type->type_flags & TypeInfoFlag_Float && right_type->type_flags & TypeInfoFlag_Float) {
        result = get_bigger_type(left_type, right_type);
    } else if (is_numeric_type(left_type) && is_numeric_type(right_type)) {
        //@Note differing types (type promotion)
        Ast_Type_Info *promotion = (left_type->type_flags & TypeInfoFlag_Float) ? left_type : right_type;
        result = promotion;
    }
    expression->inferred_type = result;
}

void Sema_Analyzer::resolve_expression(Ast_Expression *expression) {
    if (!expression) return;
    switch (expression->kind) {
    case AstKind_Ident:
    {
        Ast_Ident *ident = static_cast<Ast_Ident *>(expression);
        Ast_Declaration *lookup = scope_lookup(current_scope, ident->name);
        if (lookup) {
            resolve_declaration(lookup);
            ident->inferred_type = lookup->inferred_type;
        } else {
            error(ident->start, "undeclared identifier '%s'", ident->name->name);
            poison(ident);
        }
        break;
    }

    case AstKind_Literal:
    {
        Ast_Literal *literal = static_cast<Ast_Literal *>(expression);
        Ast_Type_Info *type_info = nullptr;
        if (literal->literal_flags & LiteralFlag_Number) {
            if (literal->literal_flags & LiteralFlag_Float) {
                type_info = t_float32;
            } else {
                type_info = t_int32; 
            }
        }
        else if (literal->literal_flags & LiteralFlag_String) {
            type_info = t_string;
        }
        literal->inferred_type = type_info;
        break;
    }

    case AstKind_UnaryExpression:
    {
        Ast_Unary_Expression *unary = static_cast<Ast_Unary_Expression *>(expression);
        resolve_expression(unary->expression);
        if (unary->expression->poisoned) {
            poison(unary);
            break;
        }
        break;
    }

    case AstKind_BinaryExpression:
    {
        int **x = (int**)(int *)expression;
        Ast_Binary_Expression *binary = static_cast<Ast_Binary_Expression *>(expression);
        resolve_expression(binary->lhs);
        resolve_expression(binary->rhs);

        if (binary->lhs->poisoned || binary->rhs->poisoned) {
            poison(binary);
            break;
        }

        if (is_arithmetic_op(binary->op)) {
            typecheck_arithmetic_expression(binary);
        }

        // -- Type Checking
        // unary
        // '-': makes sense for numeric types (e.g int, float)
        // '+': makes sense for numeric and pointer types

        // binary
        // -, +  : makes sense for numeric and pointer types
        // *, /  : makes sense for numeric types
        
        bool valid_check = true;
        Ast_Type_Info *lhs = binary->lhs->inferred_type;
        Ast_Type_Info *rhs = binary->rhs->inferred_type;
        assert(lhs && rhs);

        while (lhs || rhs) {
            if (lhs && rhs) {
                // eg. int = *int
                if (is_pointer_type(lhs) && !is_pointer_type(rhs) ||
                    !is_pointer_type(lhs) && is_pointer_type(rhs)) {
                    valid_check = false;
                    break;
                }
                // eg. int = struct
                if (is_basic_type(lhs) && !is_basic_type(rhs) ||
                    !is_basic_type(lhs) && is_basic_type(rhs)) {
                    valid_check = false;
                    break;
                }
            } else {
                // @note different levels of indirection
                valid_check = false;
                break;
            }
            lhs = lhs->base;
            rhs = rhs->base;
        }

        char *lhs_str = type_to_string(binary->lhs->inferred_type);
        char *rhs_str = type_to_string(binary->rhs->inferred_type);

        if (is_assign_operator(binary->op)) {
            if (!valid_check) {
                error(binary->start, "cannot convert from '%s' to '%s'", lhs_str, rhs_str);
            }

            if (!(binary->lhs->expr_flags & ExprFlag_Lvalue)) {
                error(binary->lhs->start, "left operand must be l-value");
                poison(binary);
            }

            //@Note if lhs is lvalue then whole expression is lvalue
            if (binary->lhs->expr_flags & ExprFlag_Lvalue) {
                binary->expr_flags |= ExprFlag_Lvalue;
            }
        } else {
            if (!valid_check) {
                error(binary->start, "invalid operands for binary '%s', '%s' and '%s'", token_type_to_string(binary->op), lhs_str, rhs_str);
            }
        }

        if (valid_check) {
            binary->inferred_type = binary->lhs->inferred_type;
        } else {
            poison(binary);
        }

        free(lhs_str);
        free(rhs_str);
        break; 
    }

    case AstKind_CallExpression:
    {
        Ast_Call_Expression *call_expr = static_cast<Ast_Call_Expression*>(expression);
        resolve_expression(call_expr->operand);
        //@Note Only identifiers are callable for now
        //@Todo Allow pointers to call functions
        if (call_expr->operand->kind != AstKind_Ident) {
            error(call_expr->operand->start, "operand must be an identifier");
            poison(call_expr);
        }

        if (call_expr->operand->inferred_type->type_kind != TypeKind_Procedure) {
            error(call_expr->operand->start, "operand must be procedure");
            poison(call_expr);
        }

        //@Note Check count and types of arguments
        if (call_expr->arguments.count != call_expr->operand->inferred_type->procedure.params.count) {
            Ast_Ident *ident = static_cast<Ast_Ident*>(call_expr->operand);
            error(call_expr->start, "'%s' does not take '%d' arguments", ident->name->name, call_expr->arguments.count);
            poison(call_expr);
        } else {
            int param_count = (int)call_expr->operand->inferred_type->procedure.params.count;
            for (int i = 0; i < param_count; i++) {
                Ast_Expression *argument = call_expr->arguments[i];
                resolve_expression(argument);
                Param_Type param_type = call_expr->operand->inferred_type->procedure.params[i];
                int x = 0;
            }
        }
        call_expr->inferred_type = call_expr->operand->inferred_type->procedure.return_type;
        break;
    }

    case AstKind_IndexExpression:
    {
        Ast_Index_Expression *index_expr = static_cast<Ast_Index_Expression*>(expression);
        resolve_expression(index_expr->array);
        resolve_expression(index_expr->index);
        if (index_expr->array->poisoned || index_expr->index->poisoned) poison(index_expr);
        if (!index_expr->array->poisoned) {
            if (!is_pointer_type(index_expr->array->inferred_type)) {
                error(index_expr->start, "array index requires array or pointer type");
                poison(index_expr);
            }
        }
        
        if (!index_expr->index->poisoned && !is_integral_type(index_expr->index->inferred_type)) {
            error(index_expr->index->start, "array index is not of integral type");
            poison(index_expr);
        }

        index_expr->inferred_type = deref_type(index_expr->array->inferred_type);
        break;
    }

    case AstKind_FieldExpression:
    {
        Ast_Field_Expression *field_expr = static_cast<Ast_Field_Expression*>(expression);
        if (field_expr->field->kind != AstKind_Ident) {
            poison(field_expr);
            error(field_expr->field->start, "right . must be a identifier");
        }

        resolve_expression(field_expr->operand);
        if (field_expr->operand->poisoned) {
            poison(field_expr);
            break;  
        }

        Ast_Type_Info *field_type = field_expr->operand->inferred_type;
        Ast_Ident *field_ident = static_cast<Ast_Ident*>(field_expr->field);
        //@Note Operand types with fields
        if (field_type->type_kind == TypeKind_Struct) {
            Ast_Type_Info *resulting_type = nullptr;
            Field_Type struct_field_type;
            Foreach(struct_field_type, field_type->aggregate.fields) {
                if (atoms_match(field_ident->name, struct_field_type.name)) {
                    resulting_type = struct_field_type.type;
                    break;
                }
            }
            field_expr->inferred_type = resulting_type;
            if (!resulting_type) {
                error(field_expr->field->start, "'%s' is not a field of '%s'", field_ident->name->name, field_type->aggregate.name->name);
                poison(field_expr);
            }
        } else if (field_type->type_kind == TypeKind_Enum || (field_type->type_kind == TypeKind_Pointer && field_type->base->type_kind == TypeKind_Struct)) {
            
        } else {
            error(field_expr->start, "left of . must have a struct/enum/pointer to struct type");
            poison(field_expr);
        }
        break; 
    }
    }
}

void Sema_Analyzer::resolve_statement(Ast_Statement *statement) {
    if (!statement) return;

    switch (statement->kind) {
    case AstKind_If:
    {
        break;
    }
    case AstKind_While:
    {
        break;
    }
    case AstKind_For:
    {
        break;
    }
    case AstKind_Return:
    {
        break;
    }
    case AstKind_Break:
    {
        break;
    }
    case AstKind_Continue:
    {
        break;
    }
    case AstKind_DeclarationStatement:
    {
        Ast_Declaration_Statement *decl_statement = static_cast<Ast_Declaration_Statement *>(statement);
        resolve_declaration(decl_statement->declaration);
        break;
    }
    case AstKind_ExpressionStatement:
    {
        Ast_Expression_Statement *expression_statement = static_cast<Ast_Expression_Statement *>(statement);
        resolve_expression(expression_statement->expression);
        break;
    }
    case AstKind_BlockStatement:
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
    if (!(declaration->declaration_flags & DeclFlag_Global)) {
        Ast_Declaration *lookup = scope_lookup(current_scope, declaration->ident->name);
        if (lookup) {
            error(lookup->ident->start, "'%s' already declared", lookup->ident->name->name);
        } else {
            scope_add_declaration(current_scope, declaration);
        }
    }

    switch (declaration->kind) {
    case AstKind_Enum:
    {
        Ast_Enum_Declaration *enum_declaration = static_cast<Ast_Enum_Declaration*>(declaration);
        for (int i = 0; i < enum_declaration->fields.count; i++) {
            Ast_Enum_Field *field = enum_declaration->fields[i];
        }
        break;
    }

    case AstKind_Struct:
    {
        Ast_Struct_Declaration *struct_declaration = static_cast<Ast_Struct_Declaration *>(declaration);
        Ast_Type_Info *type = make_type_info(TypeKind_Struct, TypeInfoFlag_Nil, 0, nullptr);
        type->aggregate.name = struct_declaration->ident->name;
        struct_declaration->inferred_type = type;
        for (int i = 0; i < struct_declaration->fields.count; i++) {
            Ast_Struct_Field *field = struct_declaration->fields[i];
            field->inferred_type = resolve_type_definition(field->type_definition);

            Field_Type field_type{};
            field_type.name = field->name;
            field_type.type = field->inferred_type;
            type->aggregate.fields.push(field_type);
        }
        struct_declaration->inferred_type = type;
        break;
    }

    case AstKind_Procedure:
    {
        Ast_Procedure_Declaration *procedure = static_cast<Ast_Procedure_Declaration *>(declaration);
        Ast_Type_Info *return_type = resolve_type_definition(procedure->return_type);
        Ast_Type_Info *proc_type = make_type_info(TypeKind_Procedure, TypeInfoFlag_Nil, 0, nullptr);
        proc_type->procedure.return_type = return_type;
        procedure->inferred_type = proc_type;

        enter_scope();
        for (int i = 0; i < procedure->parameters.count; i++) {
            Ast_Variable *param = procedure->parameters[i];
            resolve_declaration(param);

            Param_Type param_type{};
            param_type.name = param->ident->name;
            param_type.type = param->inferred_type;
            procedure->inferred_type->procedure.params.push(param_type);
        }

        for (int i = 0; i < procedure->body->statements.count; i++) {
            Ast_Statement *statement = procedure->body->statements[i];
            resolve_statement(statement);
        }
        exit_scope();
        break;
    }

    case AstKind_Variable:
    {
        Ast_Variable *variable = static_cast<Ast_Variable *>(declaration);
        assert(variable->type_definition || variable->initializer);
        
        resolve_expression(variable->initializer);
        Ast_Type_Info *defined_type = resolve_type_definition(variable->type_definition);
        Ast_Type_Info *initialized_type = variable->initializer ? variable->initializer->inferred_type : nullptr;
        variable->inferred_type = (defined_type != nullptr) ? defined_type : initialized_type;

        //@Note Check defined type and type of initializer are compatible
        if (defined_type && initialized_type) {
            if (is_numeric_type(defined_type) && !is_numeric_type(initialized_type) ||
                !is_numeric_type(defined_type) && is_numeric_type(initialized_type)) {
                char *def_s = type_to_string(defined_type);
                char *init_s = type_to_string(initialized_type);
                error(variable->start, "error initializing, cannot convert from '%s' to '%s'", init_s, def_s);
                free(def_s);
                free(init_s);
            }
            
        }

        break;
    }
    }

    declaration->resolving = false;
    declaration->resolved = true;
}
