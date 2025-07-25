#include <limits>

#include "atom.h"
#include "common.h"
#include "resolve.h"
#include "report.h"
#include "types.h"

void Resolver::resolve_star_expr(Ast_Star_Expr *star) {
    resolve_expr_base(star->elem);

    if (star->elem->valid()) {
        if (star->elem->mode == ADDRESSING_VARIABLE) {
            star->mode = ADDRESSING_VARIABLE;
            star->type = pointer_type_create(star->elem->type);
        } else if (star->elem->mode == ADDRESSING_TYPE) {
            star->mode = ADDRESSING_TYPE;
            star->type = pointer_type_create(star->elem->type);
        } else {
            CString elem_str = string_from_expr(star->elem);
            report_ast_error(star->elem, "cannot take address of '%s'.\n", elem_str);
            string_free(elem_str);
            star->poison();
        }
    } else {
        star->poison();
    }
}

void Resolver::resolve_sizeof_expr(Ast_Sizeof *size_of) {
    Ast *elem = size_of->elem;
    resolve_expr_base(elem);

    if (elem->valid()) {
        Type *type = elem->type;
        size_of->mode = ADDRESSING_CONSTANT;
        size_of->type = type_usize;
        size_of->value = constant_value_int_make(bigint_u64_make(type->size));
    } else {
        size_of->poison();
    }
}

void Resolver::resolve_builtin_binary_expr(Ast_Binary *expr) {
    Ast *lhs = expr->lhs;
    Ast *rhs = expr->rhs;
    Assert(lhs->valid() && rhs->valid());

    char *op_str = string_from_operator(expr->op);
    CString str = "";

    switch (expr->op) {
    case OP_ADD:
        if (is_pointer_type(lhs->type) && is_pointer_type(rhs->type)) {
            report_ast_error(expr, "cannot add two pointers.\n");
        } else if (is_pointer_type(lhs->type)) {
            if (!is_integral_type(rhs->type)) {
                report_ast_error(rhs, "pointer addition requires integral operand.\n");
                expr->poison();
            }
            expr->type = lhs->type;
        } else if (is_pointer_type(rhs->type)) {
            if (!is_integral_type(lhs->type)) {
                report_ast_error(lhs, "pointer addition requires integral operand.\n");
                expr->poison();
            }
            expr->type = rhs->type;
        } else {
            if (!is_numeric_type(lhs->type)) {
                str = string_from_expr(lhs);
                report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", str, op_str);
                string_free(str);
                expr->poison();
            }
            if (!is_numeric_type(rhs->type)) {
                str = string_from_expr(rhs);
                report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", str, op_str);
                string_free(str);
                expr->poison();
            }
            expr->type = lhs->type;
        }
        break;

    case OP_SUB:
        if (is_pointer_type(lhs->type) && is_pointer_type(rhs->type)) {
            if (lhs->type != rhs->type) {
                CString l_str = string_from_type(lhs->type);
                CString r_str = string_from_type(rhs->type);
                report_ast_error(expr, "'%s' and '%s' are incompatible pointer types.\n", l_str, r_str);
                string_free(l_str);
                string_free(r_str);
                expr->poison();
            }
            expr->type = lhs->type;
        } else if (is_pointer_type(lhs->type)) {
            if (!is_integral_type(rhs->type)) {
                report_ast_error(rhs, "pointer subtraction requires pointer or integral operand.\n");
                expr->poison();
            }
        } else if (is_pointer_type(rhs->type)) {
            report_ast_error(lhs, "pointer can only be subtracted from another pointer.\n");
            expr->poison();
        } else {
            if (!is_numeric_type(lhs->type)) {
                str = string_from_expr(lhs);
                report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", str, op_str);
                string_free(str);
                expr->poison();
            }
            if (!is_numeric_type(rhs->type)) {
                str = string_from_expr(rhs);
                report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", str, op_str);
                string_free(str);
                expr->poison();
            }
            expr->type = lhs->type;
        }
        break;

    case OP_MUL:
    case OP_DIV:
    case OP_MOD:
        expr->type = lhs->type;
        if (!is_numeric_type(lhs->type) || is_pointer_type(lhs->type)) {
            str = string_from_expr(lhs);
            report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", str, op_str);
            expr->poison();
        }
        if (!is_numeric_type(rhs->type) || is_pointer_type(rhs->type)) {
            str = string_from_expr(rhs);
            report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", str, op_str);
            expr->poison();
        }
        break;

    case OP_BIT_AND:
    case OP_BIT_OR:
    case OP_XOR:
    case OP_LSH:
    case OP_RSH:
        expr->type = lhs->type;
        if (!is_integral_type(lhs->type)) {
            str = string_from_expr(lhs);
            report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", str, op_str);
            string_free(str);
            expr->poison();
        }
        if (!is_integral_type(rhs->type)) {
            str = string_from_expr(rhs);
            report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", str, op_str);
            string_free(str);
            expr->poison();
        }
        break;

    case OP_EQ:
    case OP_NEQ:
    case OP_LT:
    case OP_LTEQ:
    case OP_GT:
    case OP_GTEQ:
        expr->type = type_bool;
        if (is_struct_type(lhs->type)) {
            str = string_from_expr(lhs);
            report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", str, op_str);
            string_free(str);
            expr->poison();
        }
        if (is_struct_type(rhs->type)) {
            str = string_from_expr(rhs);
            report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", str, op_str);
            string_free(str);
            expr->poison();
        }
        break;

    case OP_OR:
    case OP_AND:
        expr->type = type_bool;
        if (!is_numeric_type(lhs->type)) {
            str = string_from_expr(lhs);
            report_ast_error(lhs, "invalid operand '%s' in binary '%s'.\n", str, op_str);
            string_free(str);
            expr->poison();
        }
        if (!is_numeric_type(rhs->type)) {
            str = string_from_expr(rhs);
            report_ast_error(rhs, "invalid operand '%s' in binary '%s'.\n", str, op_str);
            string_free(str);
            expr->poison();
        }
        break;
    }

    if (expr->valid() &&
        lhs->mode == ADDRESSING_CONSTANT && rhs->mode == ADDRESSING_CONSTANT) {
        expr->mode = ADDRESSING_CONSTANT;
        expr->value = constant_binary_op_value(expr->op, expr->lhs->value, expr->rhs->value);
    }
}


void Resolver::resolve_binary_expr(Ast_Binary *expr) {
    Ast *lhs = expr->lhs;
    Ast *rhs = expr->rhs;

    resolve_expr(lhs);

    resolve_expr(rhs);

    if (lhs->valid() && rhs->valid()) {
        resolve_builtin_binary_expr(expr);  
    } else {
        expr->poison();
    }
}

void Resolver::resolve_call_expr(Ast_Call *call) {
    resolve_expr(call->elem);

    Type_Proc *tp = nullptr;
    if (call->elem->valid()) {
        if (is_proc_type(call->elem->type)) {
            tp = static_cast<Type_Proc*>(call->elem->type);
        } else {
            CString str = string_from_expr(call->elem);
            report_ast_error(call, "'%s' does not evaluate to a procedure.\n", str);
            string_free(str);
            call->poison();
            return;
        }

        call->type = tp->results;
    } else {
        call->poison();
    }

    for (Ast *arg : call->arguments) {
        resolve_expr_base(arg);
        if (arg->invalid()) {
            call->poison();
            return;
        }
    }

    int total_arg_count = get_total_value_count(call->arguments);
    int total_param_count = get_value_count(tp->params);
    if (total_arg_count < total_param_count - tp->is_variadic) {
        CString str = string_from_expr(call->elem);
        report_ast_error(call, "too few arguments for '%s', expected %d arguments, got %d.\n", str, total_param_count, total_arg_count);
        string_free(str);
        call->poison();
        return;
    }
    if (!tp->is_variadic && (total_arg_count > total_param_count)) {
        CString str = string_from_expr(call->elem);
        report_ast_error(call, "too many arguments for '%s', expected %d arguments, got %d.\n", str, total_param_count, total_arg_count);
        string_free(str);
        call->poison();
        return;
    }

    if (tp->is_variadic) {
        //@Todo Check variadic arguments
    } else {
        for (int a = 0, p = 0; a < call->arguments.count; a++) {
            Ast *arg = call->arguments[a];
            int type_count = get_value_count(arg->type);

            for (int i = 0; i < type_count; i++) {
                Type *param = tp->params->types[p];
                Type *arg_type = type_from_index(arg->type, i);
                if (!is_convertible(arg_type, param)) {
                    CString arg_str = string_from_expr(arg);
                    CString arg_type_str = string_from_type(arg_type);
                    CString param_str = string_from_type(param);
                    report_ast_error(arg, "cannot pass argument value of '%s' from type '%s' to '%s'.\n", arg_str, arg_type_str, param_str);
                    string_free(arg_str);
                    string_free(arg_type_str);
                    string_free(param_str);
                }
                p++;
            }
        }
    }
}

void Resolver::resolve_cast_expr(Ast_Cast *cast) {
    resolve_expr(cast->elem);
    cast->type = resolve_type(cast->typespec);

    if (cast->elem->valid()) {
        if (!typecheck_castable(cast->type, cast->elem->type)) {
            CString elem_str = string_from_expr(cast->elem);
            CString type_str = string_from_type(cast->type);
            CString elem_type_str = string_from_type(cast->elem->type);
            report_ast_error(cast->elem, "cannot cast '%s' as '%s' from '%s'.\n", elem_str, type_str, elem_type_str);
            string_free(elem_str);
            string_free(type_str);
            string_free(elem_type_str);
        }
        if (cast->elem->mode == ADDRESSING_CONSTANT) {
            cast->mode = ADDRESSING_CONSTANT;
            cast->value = constant_cast_value(cast->elem->value, cast->type);
        }
    } else {
        cast->poison();
    }
}

void Resolver::resolve_compound_literal(Ast_Compound_Literal *literal) {
    Type *specified_type = resolve_type(literal->typespec);

    literal->type = specified_type;

    for (int i = 0; i < literal->elements.count; i++) {
        Ast *elem = literal->elements[i];
        resolve_expr(elem);
    }

    if (is_struct_type(specified_type)) {
        Type_Struct *struct_type = (Type_Struct *)specified_type;
        // if (struct_type->fields.count < literal->elements.count) {
        //     report_ast_error(literal, "too many initializers for struct '%s'.\n", struct_type->decl->name->data);
        //     report_note(struct_type->decl->start, "see declaration of '%s'.\n", struct_type->decl->name->data);
        //     literal->poison();
        // }

        // int elem_count = Min((int)struct_type->fields.count, (int)literal->elements.count);
        // for (int i = 0; i < elem_count; i++) {
        //     Ast *elem = literal->elements[i];
        //     if (elem->invalid()) continue;

        //     Struct_Field_Info *field = &struct_type->fields[i];
        //     if (!is_convertible(field->type, elem->type)) {
        //         report_ast_error(elem, "cannot convert from '%s' to '%s'.\n", string_from_type(elem->type), string_from_type(field->type));
        //         literal->poison();
        //     }
        // }
    } else if (is_array_type(specified_type)) {
        Type *elem_type = specified_type->base;
        for (int i = 0; i < literal->elements.count; i++) {
            Ast *elem = literal->elements[i];
            if (elem->invalid()) break;

            if (!is_convertible(elem_type, elem->type)) {
                // report_ast_error(elem, "cannot convert from '%s' to '%s'.\n", string_from_type(elem->type), string_from_type(elem_type));
                literal->poison();
            }
        }
    }
}

void Resolver::resolve_deref_expr(Ast_Deref *deref) {
    resolve_expr(deref->elem);
    if (is_indirection_type(deref->elem->type)) {
        deref->type = deref->elem->type->base;
        deref->mode = ADDRESSING_VARIABLE;
    } else {
        CString str = string_from_expr(deref->elem);
        report_ast_error(deref, "cannot dereference '%s', not a pointer type.\n", str);
        string_free(str);
        deref->poison();
    }
}

void Resolver::resolve_ident(Ast_Ident *ident) {
    Decl *decl = scope_lookup(current_scope, ident->name);
    if (decl) {
        //@Note Don't resolve type decl to avoid cyclic definition error on incomplete types such as *T, []T
        if (decl->kind == DECL_TYPE && decl->resolve_state == RESOLVE_STARTED) {
            if (type_complete_path.count == 0 && !decl->type_complete) {
                report_ast_error(ident, "illegal recursive types.\n");
            }
        } else if (decl->kind == DECL_PROCEDURE) {
            resolve_proc_header(decl->proc_lit);
            decl->type = decl->proc_lit->type;
        } else {
            resolve_decl(decl);
        }

        ident->ref = decl;
        ident->type = decl->type;

        switch (decl->kind) {
        case DECL_TYPE:
            ident->mode = ADDRESSING_TYPE;
            break;
        case DECL_VARIABLE:
            ident->mode = ADDRESSING_VARIABLE;
            break;
        case DECL_CONSTANT:
            ident->mode = ADDRESSING_CONSTANT;
            ident->value = decl->constant_value;
            if (decl->constant_value.kind == CONSTANT_VALUE_TYPEID) {
                ident->mode = ADDRESSING_TYPE;
                ident->type = decl->constant_value.value_typeid;
            }
            break;
        case DECL_PROCEDURE:
            ident->mode = ADDRESSING_PROCEDURE;
            break;
        }
    } else {
        report_ast_error(ident, "undeclared identifier '%s'.\n", ident->name->data);
    }
}

bigint get_min_integer_value(Type *type) {
    Assert(is_integer_type(type));
    switch (type->kind) {
    case TYPE_UINT:
    case TYPE_UINT8:
    case TYPE_UINT16:
    case TYPE_UINT32:
    case TYPE_UINT64:
    case TYPE_USIZE:
        return bigint_make(0);
    case TYPE_INT8:
        return bigint_make(std::numeric_limits<int8_t>::min());
    case TYPE_INT16:
        return bigint_make(std::numeric_limits<int16_t>::min());
    case TYPE_INT32:
    case TYPE_INT:
        return bigint_i32_make(std::numeric_limits<int32_t>::min());
    case TYPE_INT64:
    case TYPE_ISIZE:
        return bigint_i64_make(std::numeric_limits<int64_t>::min());
    }
    return {};
}

bigint get_max_integer_value(Type *type) {
    Assert(is_integer_type(type));
    switch (type->kind) {
    case TYPE_UINT8:
        return bigint_make(std::numeric_limits<uint8_t>::max());
    case TYPE_UINT16:
        return bigint_make(std::numeric_limits<uint16_t>::max());
    case TYPE_UINT32:
    case TYPE_UINT:
        return bigint_u32_make(std::numeric_limits<uint32_t>::max());
    case TYPE_UINT64:
    case TYPE_USIZE:
        return bigint_u64_make(std::numeric_limits<uint64_t>::max());

    case TYPE_INT8:
        return bigint_make(std::numeric_limits<int8_t>::max());
    case TYPE_INT16:
        return bigint_make(std::numeric_limits<int16_t>::max());
    case TYPE_INT32:
    case TYPE_INT:
        return bigint_i32_make(std::numeric_limits<int32_t>::max());
    case TYPE_ISIZE:
    case TYPE_INT64:
        return bigint_i64_make(std::numeric_limits<int64_t>::max());
    }
    return {};
}

void Resolver::resolve_literal(Ast_Literal *literal) {
    literal->mode = ADDRESSING_CONSTANT;

    literal->value = literal->token.value;

    Type *type = nullptr;
    switch (literal->value.kind) {
    case CONSTANT_VALUE_INTEGER:
        if (literal->token.literal_kind == LITERAL_DEFAULT) {
            //@Todo Infer larger range best-fitting type based on value
            type = type_int;
        } else {
            switch (literal->token.literal_kind) {
            case LITERAL_U8: type = type_u8; break;
            case LITERAL_U16: type = type_u16; break;
            case LITERAL_U32: type = type_u32; break;
            case LITERAL_U64: type = type_u64; break;
            case LITERAL_I8:  type = type_i8; break;
            case LITERAL_I16: type = type_i16; break;
            case LITERAL_I32: type = type_i32; break;
            case LITERAL_I64: type = type_i64; break;
            }

            bigint min = get_min_integer_value(type);
            bigint max = get_max_integer_value(type);

            bool out_of_range = false;
            if (mp_cmp(&literal->value.value_integer, &min) == MP_LT) {
                out_of_range = true;
            } else if (mp_cmp(&literal->value.value_integer, &max) == MP_GT) {
                out_of_range = true;
            }

            if (out_of_range) {
                report_ast_error(literal, "invalid suffix for number literal.\n");
                report_line("\tnote: the literal '%.*s' does not fit into range of '%.*s' whose range is '%.*s'..='%.*s'.\n", LIT(literal->token.string), LIT(suffix_literals[literal->token.literal_kind].string), LIT(string_from_bigint(min)), LIT(string_from_bigint(max)));
            }
        }
        break;

    case CONSTANT_VALUE_FLOAT:
        switch (literal->token.literal_kind) {
        case LITERAL_DEFAULT: type = type_f32; break;
        case LITERAL_F32: type = type_f32; break;
        case LITERAL_F64: type = type_f64; break;
        }
        break;
        
    case CONSTANT_VALUE_STRING:
        type = type_cstring;
        break;
    }

    literal->type = type;
}

void Resolver::resolve_range_expr(Ast_Range *range) {
    resolve_expr(range->lhs);
    resolve_expr(range->rhs);

    if (range->lhs->valid()) {
        if (!is_integral_type(range->lhs->type)) {
            CString str = string_from_expr(range->lhs);
            report_ast_error(range->lhs, "'%s' is invalid range expression, not an integral type.\n", str);
            string_free(str);
            range->poison();
        }
    }

    if (range->rhs->valid()) {
        if (!is_integral_type(range->rhs->type)) {
            CString str = string_from_expr(range->rhs);
            report_ast_error(range->rhs, "'%s' is invalid range expression, not an integral type.\n", str);
            string_free(str);
            range->poison();
        }
    }

    if (range->lhs->valid() && range->rhs->valid()) {
        if (!is_convertible(range->lhs->type, range->rhs->type)) {
            CString l_str = string_from_type(range->lhs->type);
            CString r_str = string_from_type(range->rhs->type);
            report_ast_error(range, "mismatched types in range expression ('%s' and '%s').\n", l_str, r_str);
            string_free(l_str);
            string_free(r_str);
        }
    }

    if (range->lhs->mode == ADDRESSING_CONSTANT && range->rhs->mode == ADDRESSING_CONSTANT) {
        range->mode = ADDRESSING_CONSTANT;
    }

    range->type = range->lhs->type;
}

void Resolver::resolve_selector_expr(Ast_Selector *selector) {
    Ast *base = selector->parent;
    resolve_expr_base(base);

    if (base->invalid()) {
        selector->poison();
        return;
    }

    Type *type = type_deref(base->type);

    Decl *decl = lookup_field(type, selector->name->name, base->mode == ADDRESSING_TYPE);

    if (decl == nullptr) {
        if (base->mode == ADDRESSING_TYPE) {
            CString str = string_from_type(base->type);
            report_ast_error(selector->name, "'%s' is not a member of type '%s'.\n", selector->name->name->data, str);
            string_free(str);
        } else {
            CString str = string_from_expr(base);
            CString type_str = string_from_type(base->type);
            report_ast_error(selector->name, "'%s' is not a member of '%s' of type '%s'.\n", selector->name->name->data, str, type_str);
            string_free(str);
            string_free(type_str);
        }
        selector->poison();
        selector->type = type_invalid;
        selector->mode = ADDRESSING_INVALID;
        return;
    }

    selector->type = decl->type;
    selector->name->ref = decl; 

    switch (decl->kind) {
    case DECL_TYPE:
        selector->mode = ADDRESSING_TYPE;
        break;
    case DECL_VARIABLE:
        selector->mode = ADDRESSING_VARIABLE;
        break;
    case DECL_CONSTANT:
        selector->mode = ADDRESSING_CONSTANT;
        selector->value = decl->constant_value;
        break;
    case DECL_PROCEDURE:
        selector->mode = ADDRESSING_PROCEDURE;
        break;
    }
}

void Resolver::resolve_subscript_expr(Ast_Subscript *subscript) {
    resolve_expr(subscript->expr);

    resolve_expr(subscript->index);

    if (subscript->expr->valid()) {
        if (is_indirection_type(subscript->expr->type)) {
            subscript->type = type_deref(subscript->expr->type);
        } else {
            CString str = string_from_expr(subscript->expr);
            report_ast_error(subscript->expr, "'%s' is not a pointer or array type.\n", str);
            string_free(str);
            subscript->poison();
        }
    } else {
        subscript->poison();
    }

    if (subscript->index->valid()) {
        if (!is_integral_type(subscript->index->type)) {
            report_ast_error(subscript->index, "array subscript is not of integral type.\n");
            subscript->poison();
        }
    } else {
        subscript->poison();
    }

    subscript->mode = ADDRESSING_VARIABLE;
}

void Resolver::resolve_builtin_unary_expr(Ast_Unary *expr) {
    Ast *elem = expr->elem;
    char *op_str = string_from_operator(expr->op);

    switch (expr->op) {
    case OP_UNARY_PLUS:
        if (is_numeric_type(elem->type)) {
            expr->type = elem->type;
        } else {
            CString str = string_from_expr(elem);
            CString type_str = string_from_type(elem->type);
            report_ast_error(elem, "invalid operand '%s' of type '%s' in unary '%s'.\n", str, type_str, op_str);
            string_free(str);
            string_free(type_str);
            expr->poison();
        }
        break;
    case OP_UNARY_MINUS:
        if (is_numeric_type(elem->type) && !is_pointer_type(elem->type)) {
            expr->type = elem->type;
        } else {
            CString str = string_from_expr(elem);
            CString type_str = string_from_type(elem->type);
            report_ast_error(elem, "invalid operand '%s' of type '%s' in unary '%s'.\n", str, type_str, op_str);
            string_free(str);
            string_free(type_str);
            expr->poison();
        }
        break;
    case OP_NOT:
        if (is_numeric_type(elem->type)) {
            expr->type = type_bool;
        } else {
            CString str = string_from_expr(elem);
            CString type_str = string_from_type(elem->type);
            report_ast_error(elem, "invalid operand '%s' of type '%s' in unary '%s'.\n", str, type_str, op_str);
            string_free(str);
            string_free(type_str);
            expr->poison();
        }
        break;
    case OP_BIT_NOT:
        if (is_numeric_type(elem->type)) {
            expr->type = elem->type;
        } else {
            CString str = string_from_expr(elem);
            CString type_str = string_from_type(elem->type);
            report_ast_error(elem, "invalid operand '%s' of type '%s' in unary '%s'.\n", str, type_str, op_str);
            string_free(str);
            string_free(type_str);
            expr->poison();
        }
        break;
    }
}

void Resolver::resolve_unary_expr(Ast_Unary *expr) {
    Ast *elem = expr->elem;
    resolve_expr(elem);
    if (elem->valid()) {
        resolve_builtin_unary_expr(expr);
        if (expr->valid() && elem->mode == ADDRESSING_CONSTANT) {
            expr->mode = ADDRESSING_CONSTANT;
            expr->value = constant_unary_op_value(expr->op, elem->value);
        }
    } else {
        expr->poison();
    }
}

void Resolver::resolve_single_value(Ast *expr) {
    Type *type = expr->type;
    if (type && type->kind == TYPE_TUPLE) {
        Type_Tuple *tuple = (Type_Tuple *)type;
        if (tuple->types.count > 1) {
            expr->poison();
            report_ast_error(expr, "multi valued (%d) expression where single value type expected.\n", tuple->types.count);
        } else if (tuple->types.count == 1) {
            expr->type = tuple->types[0];
        } else {
            expr->type = nullptr;
        }
    }
}

void Resolver::resolve_expr(Ast *expr) {
    resolve_expr_base(expr);

    if (expr->mode == ADDRESSING_TYPE) {
        report_ast_error(expr, "expected value, not type.\n");
        expr->poison();
    }

    resolve_single_value(expr);
}

void Resolver::resolve_expr_base(Ast *expr) {
    if (!expr) return;

    switch (expr->kind) {
    case AST_SIZEOF: {
        Ast_Sizeof *size_of = static_cast<Ast_Sizeof*>(expr);
        resolve_sizeof_expr(size_of);
        return;
    }

    case AST_BINARY: {
        Ast_Binary *binary = static_cast<Ast_Binary*>(expr);
        resolve_binary_expr(binary);
        return;
    }

    case AST_CALL: {
        Ast_Call *call = static_cast<Ast_Call*>(expr);
        resolve_call_expr(call);
        return;
    }

    case AST_CAST: {
        Ast_Cast *cast = static_cast<Ast_Cast*>(expr);
        resolve_cast_expr(cast);
        return;
    }

    case AST_COMPOUND_LITERAL: {
        Ast_Compound_Literal *literal = static_cast<Ast_Compound_Literal*>(expr);
        resolve_compound_literal(literal);
        return;
    }

    case AST_DEREF: {
        Ast_Deref *deref = static_cast<Ast_Deref*>(expr);
        resolve_deref_expr(deref);
        return;
    }

    case AST_IDENT: {
        Ast_Ident *ident = (Ast_Ident *)expr;
        resolve_ident(ident);
        return;
    }

    case AST_LITERAL: {
        Ast_Literal *literal = (Ast_Literal *)expr;
        resolve_literal(literal);
        return;
    }

    case AST_UNINIT: {
        expr->type = type_uninit_value;
        expr->mode = ADDRESSING_VALUE;
        report_ast_error(expr, "--- used outside of variable declaration.\n");
        return;
    }

    case AST_PAREN: {
        Ast_Paren *paren = static_cast<Ast_Paren*>(expr);
        resolve_expr_base(paren->elem);

        if (paren->elem->valid()) {
            paren->mode = paren->elem->mode;
            paren->value = paren->elem->value;
            paren->type = paren->elem->type;
        } else {
            paren->poison();
        }
        return;
    }

    case AST_PROC_LIT:
        resolve_proc_lit((Ast_Proc_Lit *)expr);
        return;

    case AST_RANGE: {
        Ast_Range *range = static_cast<Ast_Range*>(expr);
        resolve_range_expr(range);
        return;
    }

    case AST_SELECTOR: {
        Ast_Selector *selector = static_cast<Ast_Selector*>(expr);
        resolve_selector_expr(selector);
        return;
    }

    case AST_STAR_EXPR: {
        Ast_Star_Expr *star_expr = static_cast<Ast_Star_Expr*>(expr);
        resolve_star_expr(star_expr);
        return;
    }

    case AST_ARRAY_TYPE:
        resolve_type(expr);
        return;

    case AST_STRUCT_TYPE:
        resolve_struct_type((Ast_Struct_Type *)expr);
        return;

    case AST_UNION_TYPE:
        resolve_union_type((Ast_Union_Type *)expr);
        return;

    case AST_PROC_TYPE:
        resolve_proc_type((Ast_Proc_Type *)expr, false);
        return;

    case AST_ENUM_TYPE:
        resolve_enum_type((Ast_Enum_Type *)expr);
        return;

    case AST_SUBSCRIPT: {
        Ast_Subscript *subscript = static_cast<Ast_Subscript *>(expr);
        resolve_subscript_expr(subscript);
        return;
    }

    case AST_UNARY: {
        Ast_Unary *unary = static_cast<Ast_Unary*>(expr);
        resolve_unary_expr(unary);
        return;
    }
    }

    Assert(0); // unreachable
}
