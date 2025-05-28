global Ast_Type_Info *g_builtin_types[BUILTIN_TYPE_COUNT];

global Ast_Type_Info *type_poison;
global Ast_Type_Info *type_void;
global Ast_Type_Info *type_null;
global Ast_Type_Info *type_bool;
global Ast_Type_Info *type_u8, *type_u16, *type_u32, *type_u64, *type_uint;
global Ast_Type_Info *type_i8, *type_i16, *type_i32, *type_i64, *type_int;
global Ast_Type_Info *type_isize, *type_usize;
global Ast_Type_Info *type_f32, *type_f64;
global Ast_Type_Info *type_string;

internal Ast_Type_Info *ast_type_info(Atom *name, Type_Info_Flags flags) {
    Ast_Type_Info *result = AST_NEW(Ast_Type_Info);
    result->name = name; 
    result->type_flags = flags;
    return result;
}

internal Ast_Type_Info *ast_pointer_type_info(Ast_Type_Info *base) {
    Ast_Type_Info *result = AST_NEW(Ast_Type_Info);
    result->base = base;
    result->type_flags = TYPE_FLAG_POINTER;
    return result;
}

internal Struct_Field_Info struct_field_info(Atom *name, Ast_Type_Info *type_info) {
    Struct_Field_Info result = {};
    result.name = name;
    result.type_info = type_info;
    result.mem_offset = 0;
    return result;
}

internal Ast_Array_Type_Info *ast_array_type_info(Ast_Type_Info *base) {
    Ast_Array_Type_Info *result = AST_NEW(Ast_Array_Type_Info);
    result->base = base;
    result->type_flags = TYPE_FLAG_ARRAY;
    result->aggregate.fields = {
        struct_field_info(atom_create(str_lit("data")), ast_pointer_type_info(base)),
        struct_field_info(atom_create(str_lit("count")), type_i64)
    };
    return result;
}

internal Ast_Proc_Type_Info *ast_proc_type_info(Ast_Type_Info *return_type, Auto_Array<Ast_Type_Info*> parameters) {
    Ast_Proc_Type_Info *result = AST_NEW(Ast_Proc_Type_Info);
    result->type_flags = TYPE_FLAG_PROC;
    result->return_type = return_type;
    result->parameters = parameters;
    return result;
}

internal Ast_Type_Info *ast_struct_type_info(Auto_Array<Struct_Field_Info> fields) {
    Ast_Type_Info *result = AST_NEW(Ast_Type_Info);
    result->type_flags = TYPE_FLAG_STRUCT;
    result->aggregate.fields = fields;
    return result;
}

internal Ast_Enum_Type_Info *ast_enum_type_info(Auto_Array<Enum_Field_Info> fields) {
    Ast_Enum_Type_Info *result = AST_NEW(Ast_Enum_Type_Info);
    result->type_flags = TYPE_FLAG_ENUM;
    result->fields = fields;
    return result;
}

internal Ast_Type_Info *ast_builtin_type(Builtin_Type_Kind builtin_kind, String8 name, int bytes, Type_Info_Flags flags) {
    Atom *atom = atom_create(name);
    Ast_Type_Info *result = ast_type_info(atom, flags | TYPE_FLAG_BUILTIN);
    result->bytes = bytes;
    g_builtin_types[builtin_kind] = result;
    result->builtin_kind = builtin_kind;
    return result;
}

internal void register_builtin_types() {
    int system_max_bytes = 8;

    type_poison = ast_builtin_type(BUILTIN_TYPE_POISON, str_lit("builtin(poison)"), 0, TYPE_FLAG_POISON);
    type_void   = ast_builtin_type(BUILTIN_TYPE_VOID,   str_lit("void"), 0, TYPE_FLAG_VOID);
    type_null   = ast_builtin_type(BUILTIN_TYPE_NULL,   str_lit("builtin(null)"), -1, TYPE_FLAG_NULL);
    type_bool   = ast_builtin_type(BUILTIN_TYPE_BOOL,   str_lit("bool"), 1, TYPE_FLAG_INTEGER | TYPE_FLAG_BOOLEAN);

    type_u8  = ast_builtin_type(BUILTIN_TYPE_U8,   str_lit("u8"),   1, TYPE_FLAG_INTEGER);
    type_u16 = ast_builtin_type(BUILTIN_TYPE_U16,  str_lit("u16"),  2, TYPE_FLAG_INTEGER);
    type_u32 = ast_builtin_type(BUILTIN_TYPE_U32,  str_lit("u32"),  4, TYPE_FLAG_INTEGER);
    type_u64 = ast_builtin_type(BUILTIN_TYPE_U64,  str_lit("u64"),  8, TYPE_FLAG_INTEGER);

    type_i8   = ast_builtin_type(BUILTIN_TYPE_I8,   str_lit("i8"),   1, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_i16  = ast_builtin_type(BUILTIN_TYPE_I16,  str_lit("i16"),  2, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_i32  = ast_builtin_type(BUILTIN_TYPE_I32,  str_lit("i32"),  4, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_i64  = ast_builtin_type(BUILTIN_TYPE_I64,  str_lit("i64"),  8, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);

    type_f32  = ast_builtin_type(BUILTIN_TYPE_F32,  str_lit("f32"),  4, TYPE_FLAG_FLOAT);
    type_f64  = ast_builtin_type(BUILTIN_TYPE_F64,  str_lit("f64"),  8, TYPE_FLAG_FLOAT);

    type_uint = ast_builtin_type(BUILTIN_TYPE_UINT,  str_lit("uint"),  4, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_int  = ast_builtin_type(BUILTIN_TYPE_INT,   str_lit("int"),  4, TYPE_FLAG_INTEGER);

    type_usize = ast_builtin_type(BUILTIN_TYPE_USIZE, str_lit("usize"), system_max_bytes, TYPE_FLAG_INTEGER);
    type_isize = ast_builtin_type(BUILTIN_TYPE_ISIZE, str_lit("isize"), system_max_bytes, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);

    {
        type_string = ast_builtin_type(BUILTIN_TYPE_STRING, str_lit("string"), 16, TYPE_FLAG_STRING);
        type_string->aggregate.fields = {
            { atom_create(str_lit("data")), ast_pointer_type_info(type_u8) },
            { atom_create(str_lit("count")), type_i32 }
        };
    }
}

//@Todo More robust type checking for non-indirection types that are "aggregate" such as struct and procedure types.
//      For now we just check if they are identical, not equivalent.
internal bool typecheck(Ast_Type_Info *t0, Ast_Type_Info *t1) {
    Assert(t0 != NULL);
    Assert(t1 != NULL);

    if (t0 == t1) return true;

    //@Note Any results of poisoned types, just okay it
    if (t0->is_poisoned || t1->is_poisoned) return true;

    //@Note Nullable types
    if (t1->type_flags & TYPE_FLAG_NULL) {
        if (t0->is_indirection_type()) {
            return true;
        } else {
            return false;
        }
    }

    //@Note Indirection testing
    if (t0->is_indirection_type() && t1->is_integral_type()) {
        return true;
    }

    if (t0->is_indirection_type() != t1->is_indirection_type()) {
        return false;
    }

    if (t0->is_indirection_type() && t1->is_indirection_type()) {
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
    }

    //@Note Should have early returned on identical check
    if (t0->is_enum_type() || t1->is_enum_type()) {
        return false;
    }

    if (t0->is_integral_type() == t1->is_integral_type()) {
        if (t0->bytes == t1->bytes) {
            return true;
        }
    }

    return false;
}

internal bool typecheck_castable(Ast_Type_Info *t0, Ast_Type_Info *t1) {
    Assert(t0 != NULL);
    Assert(t1 != NULL);

    if (t0->is_indirection_type() != t1->is_indirection_type()) {
        return false; 
    }

    if (!t0->is_indirection_type() && !t1->is_indirection_type()) {
        if (t0->is_struct_type() != t1->is_struct_type()) {
            return false;
        }
    }

    return true;
}

bool Ast_Type_Info::is_struct_access() {
    if (this->is_struct_type() ||
        (is_pointer_type() && this->base->is_struct_type())) {
        return true;
    }
    return false;
}
