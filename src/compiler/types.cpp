global Ast_Type_Info *g_builtin_types[BUILTIN_TYPE_COUNT];

global Ast_Type_Info *type_poison;
global Ast_Type_Info *type_void;

global Ast_Type_Info *type_u8;
global Ast_Type_Info *type_u16;
global Ast_Type_Info *type_u32;
global Ast_Type_Info *type_u64;

global Ast_Type_Info *type_s8;
global Ast_Type_Info *type_s16;
global Ast_Type_Info *type_s32;
global Ast_Type_Info *type_bool;

global Ast_Type_Info *type_s64;
global Ast_Type_Info *type_f32;
global Ast_Type_Info *type_f64;

internal Ast_Type_Info *ast_type_info(Atom *name, Type_Info_Flags flags) {
    Ast_Type_Info *result = AST_NEW(Ast_Type_Info);
    result->name = name; 
    result->type_flags = flags;
    return result;
}

internal Ast_Type_Info *ast_builtin_type(Builtin_Type_Kind builtin_kind, String8 name, int bytes, Type_Info_Flags flags) {
    Atom *atom = atom_create(name);
    Ast_Type_Info *result = ast_type_info(atom, flags | TYPE_FLAG_BUILTIN);
    result->bytes = bytes;
    g_builtin_types[builtin_kind] = result;
    return result;
}

internal void register_builtin_types() {
    type_poison = ast_type_info(NULL, TYPE_FLAG_POISON);
    type_void = ast_builtin_type(BUILTIN_TYPE_VOID, str8_lit("void"), 0, TYPE_FLAG_NIL);
    type_u8  = ast_builtin_type(BUILTIN_TYPE_U8,  str8_lit("u8"),  1, TYPE_FLAG_INTEGER);
    type_u16 = ast_builtin_type(BUILTIN_TYPE_U16, str8_lit("u16"), 2, TYPE_FLAG_INTEGER);
    type_u32 = ast_builtin_type(BUILTIN_TYPE_U32, str8_lit("u32"), 4, TYPE_FLAG_INTEGER);
    type_u64 = ast_builtin_type(BUILTIN_TYPE_U64, str8_lit("u64"), 8, TYPE_FLAG_INTEGER);
    type_s8  = ast_builtin_type(BUILTIN_TYPE_S8,  str8_lit("s8"),  1, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_s16 = ast_builtin_type(BUILTIN_TYPE_S16, str8_lit("s16"), 2, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_s32 = ast_builtin_type(BUILTIN_TYPE_S32, str8_lit("s32"), 4, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_s64 = ast_builtin_type(BUILTIN_TYPE_S64, str8_lit("s64"), 8, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_bool = ast_builtin_type(BUILTIN_TYPE_BOOL, str8_lit("bool"), 4, TYPE_FLAG_INTEGER | TYPE_FLAG_BOOLEAN);
    type_f32 = ast_builtin_type(BUILTIN_TYPE_F32, str8_lit("f32"), 4, TYPE_FLAG_FLOAT);
    type_f64 = ast_builtin_type(BUILTIN_TYPE_F64, str8_lit("f64"), 8, TYPE_FLAG_FLOAT);
}

internal inline bool is_struct_type(Ast_Type_Info *type) {
    return type->type_flags & TYPE_FLAG_STRUCT;
}

internal inline bool is_conditional_type(Ast_Type_Info *type) {
    return (type->type_flags & TYPE_FLAG_NUMERIC) || (type->type_flags & TYPE_FLAG_POINTER) || (type->type_flags & TYPE_FLAG_ENUM);
}

internal inline bool is_indirection_type(Ast_Type_Info *type) {
    return type->type_flags & TYPE_FLAG_POINTER || type->type_flags & TYPE_FLAG_ARRAY;
}

internal inline bool is_arithmetic_type(Ast_Type_Info *type) {
    return type->type_flags & TYPE_FLAG_POINTER || type->type_flags & TYPE_FLAG_NUMERIC || type->type_flags & TYPE_FLAG_ENUM;
}

internal inline bool is_integral_type(Ast_Type_Info *type) {
    return type->type_flags & TYPE_FLAG_INTEGRAL;
}

internal inline Ast_Type_Info *deref_type(Ast_Type_Info *type) {
    return type->base;
}

//@Todo More robust type checking for non-indirection types that are "aggregate" such as struct and procedure types.
//      For now we just check if they are identical, not equivalent.
internal bool typecheck(Ast_Type_Info *t0, Ast_Type_Info *t1) {
    Assert(t0 != NULL);
    Assert(t1 != NULL);

    //@Note Any results of poisoned types, just okay it
    if (t0->is_poisoned || t1->is_poisoned) return true;

    //@Note Indirection testing
    if (is_indirection_type(t0) != is_indirection_type(t1)) {
        return false;
    } else if (is_indirection_type(t0) && is_indirection_type(t1)) {
        Ast_Type_Info *a = t0, *b = t1;
        for (;;) {
            //@Note Bad indirection
            if ((a == NULL) != (b == NULL)) {
                return false;
            }

            if (a->base == NULL && b->base == NULL) {
                return a == b;
            }

            a = a->base;
            b = b->base;
        }
    } else {
        return t0 == t1;
    }
}

internal bool typecheck_castable(Ast_Type_Info *t0, Ast_Type_Info *t1) {
    Assert(t0 != NULL);
    Assert(t1 != NULL);

    if ((is_indirection_type(t0) != NULL) != (is_indirection_type(t1) != NULL)) {
        return false; 
    }

    if (!is_indirection_type(t0) && !is_indirection_type(t1)) {
        if (is_struct_type(t0) != is_struct_type(t1)) {
            return false;
        }
    }

    return true;
}

