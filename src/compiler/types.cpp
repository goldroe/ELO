global Auto_Array<Type*> g_builtin_types;
global Type *type_poison;
global Type *type_void;
global Type *type_null;
global Type *type_bool;
global Type *type_u8, *type_u16, *type_u32, *type_u64, *type_uint;
global Type *type_i8, *type_i16, *type_i32, *type_i64, *type_int;
global Type *type_isize, *type_usize;
global Type *type_f32, *type_f64;
global Type *type_string;

internal Type *pointer_type(Type *base) {
    Type *result = AST_NEW(Type);
    result->base = base;
    result->id = TYPEID_POINTER;
    return result;
}

internal Struct_Field_Info struct_field_info(Atom *name, Type *type) {
    Struct_Field_Info result = {};
    result.name = name;
    result.type = type;
    result.mem_offset = 0;
    return result;
}

internal Array_Type *array_type(Type *base) {
    Array_Type *result = AST_NEW(Array_Type);
    result->base = base;
    result->id = TYPEID_ARRAY;
    result->aggregate.fields = {
        struct_field_info(atom_create(str_lit("data")), pointer_type(base)),
        struct_field_info(atom_create(str_lit("count")), type_i64)
    };
    return result;
}

internal Proc_Type *proc_type(Type *return_type, Auto_Array<Type*> parameters) {
    Proc_Type *result = AST_NEW(Proc_Type);
    result->id = TYPEID_PROC;
    result->return_type = return_type;
    result->parameters = parameters;
    return result;
}

internal Type *struct_type(Auto_Array<Struct_Field_Info> fields) {
    Type *result = AST_NEW(Type);
    result->id = TYPEID_STRUCT;
    result->aggregate.fields = fields;
    return result;
}

internal Enum_Type *enum_type(Auto_Array<Enum_Field_Info> fields) {
    Enum_Type *result = AST_NEW(Enum_Type);
    result->id = TYPEID_ENUM;
    result->fields = fields;
    return result;
}

internal Type *builtin_type_create(Type_ID type_id, String8 name, int bytes, Type_Flags flags = (Type_Flags)0) {
    Atom *atom = atom_create(name);
    Type *type = AST_NEW(Type);
    type->name = atom;
    type->id = type_id;
    type->type_flags = flags;
    type->bytes = bytes;
    g_builtin_types.push(type);
    return type;
}

internal void register_builtin_types() {
    int system_max_bytes = 8;
    type_poison = builtin_type_create(TYPEID_POISON, str_lit("builtin(poison)"), 0);//, TYPE_FLAG_POISON);
    type_void   = builtin_type_create(TYPEID_VOID,   str_lit("void"), 0);//, TYPE_FLAG_VOID);
    type_null   = builtin_type_create(TYPEID_NULL,   str_lit("builtin(null)"), 0);//, TYPE_FLAG_NULL);

    type_u8    = builtin_type_create(TYPEID_UINT8,    str_lit("u8"),     1);//, TYPE_FLAG_INTEGER);
    type_u16   = builtin_type_create(TYPEID_UINT16,   str_lit("u16"),    2);//, TYPE_FLAG_INTEGER);
    type_u32   = builtin_type_create(TYPEID_UINT32,   str_lit("u32"),    4);//, TYPE_FLAG_INTEGER);
    type_u64   = builtin_type_create(TYPEID_UINT64,   str_lit("u64"),    8);//, TYPE_FLAG_INTEGER);
    type_i8    = builtin_type_create(TYPEID_INT8,     str_lit("i8"),     1);//, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_i16   = builtin_type_create(TYPEID_INT16,    str_lit("i16"),    2);//, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_i32   = builtin_type_create(TYPEID_INT32,    str_lit("i32"),    4);//, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_i64   = builtin_type_create(TYPEID_INT64,    str_lit("i64"),    8);//, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_uint  = builtin_type_create(TYPEID_UINT,     str_lit("uint"),   4);//, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_int   = builtin_type_create(TYPEID_INT,      str_lit("int"),    4);//, TYPE_FLAG_INTEGER);
    type_bool  = builtin_type_create(TYPEID_BOOL,     str_lit("bool"),   1);//, TYPE_FLAG_INTEGER | TYPE_FLAG_BOOLEAN);
    type_usize = builtin_type_create(TYPEID_USIZE,    str_lit("usize"),  8);//, TYPE_FLAG_INTEGER);
    type_isize = builtin_type_create(TYPEID_ISIZE,    str_lit("isize"),  8);//, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_f32   = builtin_type_create(TYPEID_FLOAT32,  str_lit("f32"),    4);//, TYPE_FLAG_FLOAT);
    type_f64   = builtin_type_create(TYPEID_FLOAT64,  str_lit("f64"),    8);//, TYPE_FLAG_FLOAT);

    {
        type_string = builtin_type_create(TYPEID_STRING, str_lit("string"), 16);
        type_string->aggregate.fields = {
            { atom_create(str_lit("data")), pointer_type(type_u8) },
            { atom_create(str_lit("count")), type_i32 }
        };
    }
}

//@Todo More robust type checking for non-indirection types that are "aggregate" such as struct and procedure types.
//      For now we just check if they are identical, not equivalent.
internal bool typecheck(Type *t0, Type *t1) {
    Assert(t0 != NULL);
    Assert(t1 != NULL);

    if (t0 == t1) return true;

    //@Note Any results of poisoned types, just okay it
    if (t0->is_poisoned || t1->is_poisoned) return true;

    //@Note Nullable types
    if (t1->id == TYPEID_NULL) {
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
        Type *a = t0, *b = t1;
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

internal bool typecheck_castable(Type *t0, Type *t1) {
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

