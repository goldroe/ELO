global Auto_Array<Type*> g_builtin_types;
global Type *type_invalid;
global Type *type_void;
global Type *type_null;
global Type *type_bool;
global Type *type_u8, *type_u16, *type_u32, *type_u64, *type_uint;
global Type *type_i8, *type_i16, *type_i32, *type_i64, *type_int;
global Type *type_isize, *type_usize;
global Type *type_f32, *type_f64;
global Type *type_string;

internal Pointer_Type *pointer_type_create(Type *elem) {
    Pointer_Type *type = TYPE_NEW(Pointer_Type);
    type->base = elem;
    return type;
}

internal Array_Type *array_type_create(Type *elem) {
    Array_Type *type = TYPE_NEW(Array_Type);
    type->base = elem;
    return type;
}

internal Tuple_Type *tuple_type_create(Auto_Array<Type*> types) {
    Tuple_Type *type = TYPE_NEW(Tuple_Type);
    type->types = types;
    return type;
}

internal Proc_Type *proc_type_create(Auto_Array<Type*> params, Auto_Array<Type*> results) {
    Proc_Type *type = TYPE_NEW(Proc_Type);
    type->params = tuple_type_create(params);
    type->results = tuple_type_create(results);
    return type;
}

internal Struct_Type *struct_type_create(Atom *name, Auto_Array<Decl*> members, Scope *scope) {
    Struct_Type *type = TYPE_NEW(Struct_Type);
    type->name = name;
    type->members = members;
    type->scope = scope;
    return type;
}

internal Enum_Type *enum_type_create(Type *base_type, Auto_Array<Decl*> fields, Scope *scope) {
    Enum_Type *type = TYPE_NEW(Enum_Type);
    type->base_type = type;
    type->fields = fields;
    type->scope = scope;
    return type;
}

internal Type *builtin_type_create(Type_Kind kind, String8 name, int bytes, Type_Flags flags = (Type_Flags)0) {
    Atom *atom = atom_create(name);
    Type *type = TYPE_NEW(Type);
    type->name = atom;
    type->kind = kind;
    type->type_flags = flags;
    type->bytes = bytes;
    g_builtin_types.push(type);
    return type;
}

internal void register_builtin_types() {
    int system_max_bytes = 8;
    type_invalid = builtin_type_create(TYPE_INVALID, str_lit("builtin(invalid)"), 0);
    type_void    = builtin_type_create(TYPE_VOID,    str_lit("void"),  0);//, TYPE_FLAG_VOID);
    type_u8    = builtin_type_create(TYPE_UINT8,    str_lit("u8"),     1);//, TYPE_FLAG_INTEGER);
    type_u16   = builtin_type_create(TYPE_UINT16,   str_lit("u16"),    2);//, TYPE_FLAG_INTEGER);
    type_u32   = builtin_type_create(TYPE_UINT32,   str_lit("u32"),    4);//, TYPE_FLAG_INTEGER);
    type_u64   = builtin_type_create(TYPE_UINT64,   str_lit("u64"),    8);//, TYPE_FLAG_INTEGER);
    type_i8    = builtin_type_create(TYPE_INT8,     str_lit("i8"),     1);//, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_i16   = builtin_type_create(TYPE_INT16,    str_lit("i16"),    2);//, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_i32   = builtin_type_create(TYPE_INT32,    str_lit("i32"),    4);//, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_i64   = builtin_type_create(TYPE_INT64,    str_lit("i64"),    8);//, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_uint  = builtin_type_create(TYPE_UINT,     str_lit("uint"),   4);//, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_int   = builtin_type_create(TYPE_INT,      str_lit("int"),    4);//, TYPE_FLAG_INTEGER);
    type_bool  = builtin_type_create(TYPE_BOOL,     str_lit("bool"),   1);//, TYPE_FLAG_INTEGER | TYPE_FLAG_BOOLEAN);
    type_usize = builtin_type_create(TYPE_USIZE,    str_lit("usize"),  8);//, TYPE_FLAG_INTEGER);
    type_isize = builtin_type_create(TYPE_ISIZE,    str_lit("isize"),  8);//, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED);
    type_f32   = builtin_type_create(TYPE_FLOAT32,  str_lit("f32"),    4);//, TYPE_FLAG_FLOAT);
    type_f64   = builtin_type_create(TYPE_FLOAT64,  str_lit("f64"),    8);//, TYPE_FLAG_FLOAT);
    type_null  = pointer_type_create(type_void);

    type_string = builtin_type_create(TYPE_STRING, str_lit("string"), 16);
    {
    //     type_string->aggregate.fields = {
    //         { atom_create(str_lit("data")), pointer_type(type_u8) },
    //         { atom_create(str_lit("count")), type_i32 }
    //     };
    }
}

//@Todo More robust type checking for non-indirection types that are "aggregate" such as struct and procedure types.
//      For now we just check if they are identical, not equivalent.
internal bool is_convertible(Type *t0, Type *t1) {
    Assert(t0 != NULL);
    Assert(t1 != NULL);

    if (t0 == t1) return true;

    //@Note Any results of poisoned types, just okay it
    // if (t0->is_poisoned || t1->is_poisoned) return true;

    //@Note Nullable types
    if (t1->kind == TYPE_NULL) {
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
    Assert(t0 && t1);

    if (t0 == t1) return true;

    if (t0->is_numeric_type() && t1->is_numeric_type()) {
        return true;
    } else if (t0->is_integer_type()) {
        return t1->is_pointer_like_type();
    } else if (t1->is_integer_type()) {
        return t0->is_pointer_like_type();
    } else if (t0->is_pointer_like_type() && t1->is_pointer_like_type()) {
        return true;
    } else {
        return false;
    }
}


internal Type *type_deref(Type *t) {
    if (t) {
        if (t->base) {
            return t->base;
        }
    }
    return t;
}


internal char *string_from_type(Type *ty) {
    cstring string = NULL;
    if (ty == NULL) return "";

    for (Type *type = ty; type; type = type->base) {
        switch (type->kind) {
        case TYPE_POINTER:
            cstring_append(&string, "*");
            break;

        case TYPE_ARRAY:
            cstring_append(&string, "[..]");
            break;

        case TYPE_TUPLE: {
            Tuple_Type *tuple_type = (Tuple_Type *)type;
            for (Type *type : tuple_type->types) {
                cstring_append(&string, string_from_type(type));
                if (type == tuple_type->types.back()) cstring_append(&string, ",");
            }
            break;
        }

        case TYPE_PROC: {
            Proc_Type *proc_type = static_cast<Proc_Type*>(type);
            cstring_append(&string, "(");

            cstring_append(&string, ")");

            if (proc_type->results) {
                cstring_append(&string, "->");
                cstring_append(&string, string_from_type(proc_type->results));
            }
            break;
        }

        case TYPE_ENUM: {
            Enum_Type *enum_type = static_cast<Enum_Type*>(type);
            if (enum_type->name) {
                cstring_append(&string, enum_type->name->data);
            } else {
                cstring_append(&string, "<anon enum>");
            }
            break;
        }

        case TYPE_STRUCT: {
            Struct_Type *struct_type = static_cast<Struct_Type*>(type);
            if (struct_type->name) {
                cstring_append(&string, struct_type->name->data);
            } else {
                cstring_append(&string, "<anon struct>");
            }
            break;
        }

        case TYPE_UNION: {
            Union_Type *union_type = static_cast<Union_Type*>(type);
            if (union_type->name) {
                cstring_append(&string, union_type->name->data);
            } else {
                cstring_append(&string, "<anon union>");
            }
            break;
        }

        default:
            cstring_append(&string, type->name->data);
            break;
        }
    }
    return string;
}

