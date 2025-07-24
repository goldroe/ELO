#include "atom.h"
#include "base/base_strings.h"
#include "types.h"

Array<Type*> g_builtin_types;
Type *type_invalid;
Type *type_void;
Type *type_null;
Type *type_uninit_value;
Type *type_bool;
Type *type_u8, *type_u16, *type_u32, *type_u64, *type_uint;
Type *type_i8, *type_i16, *type_i32, *type_i64, *type_int;
Type *type_isize, *type_usize;
Type *type_f32, *type_f64;
Type *type_string;
Type *type_cstring;
Type *type_any;

Type_Pointer *pointer_type_create(Type *elem) {
    Type_Pointer *type = TYPE_NEW(Type_Pointer);
    type->base = elem;
    return type;
}

Type_Array *array_type_create(Type *elem, u64 array_size) {
    Type_Array *type = TYPE_NEW(Type_Array);
    type->base = elem;
    type->array_size = array_size;
    return type;
}

Type_Array_View *array_view_type_create(Type *elem) {
    Type_Array_View *type = TYPE_NEW(Type_Array_View);
    type->base = elem;
    return type;
}

Type_Dynamic_Array *dynamic_array_type_create(Type *elem) {
    Type_Dynamic_Array *type = TYPE_NEW(Type_Dynamic_Array);
    type->base = elem;
    return type;
}

Type_Tuple *tuple_type_create(Array<Type*> types) {
    Type_Tuple *type = TYPE_NEW(Type_Tuple);
    type->types = types;
    return type;
}

Type_Proc *proc_type_create(Array<Type*> params, Array<Type*> results) {
    Type_Proc *type = TYPE_NEW(Type_Proc);
    type->params = tuple_type_create(params);
    type->results = tuple_type_create(results);
    return type;
}

Type_Struct *struct_type_create(Atom *name, Array<Decl*> members, Scope *scope) {
    Type_Struct *type = TYPE_NEW(Type_Struct);
    type->name = name;
    type->members = members;
    type->scope = scope;
    return type;
}

Type_Union *union_type_create(Atom *name, Array<Decl*> members, Scope *scope) {
    Type_Union *type = TYPE_NEW(Type_Union);
    type->name = name;
    type->members = members;
    type->scope = scope;
    return type;
}

Type_Enum *enum_type_create(Type *base_type, Array<Decl*> fields, Scope *scope) {
    Type_Enum *type = TYPE_NEW(Type_Enum);
    type->base_type = base_type;
    type->fields = fields;
    type->scope = scope;
    return type;
}

Type *builtin_type_create(Type_Kind kind, String name, int size) {
    Atom *atom = atom_create(name);
    Type *type = TYPE_NEW(Type);
    type->name = atom;
    type->kind = kind;
    type->size = size;
    array_add(&g_builtin_types, type);
    return type;
}

i64 size_from_type(Type *type) {
    switch (type->kind) {
    case TYPE_STRUCT: {
        break;
    }
    }
    return 0;
}

void register_builtin_types() {
    int system_max_bytes = 8;
    array_init(&g_builtin_types, heap_allocator());
    type_invalid = builtin_type_create(TYPE_INVALID, str_lit("builtin(invalid)"), 0);
    type_void    = builtin_type_create(TYPE_VOID,    str_lit("void"),  0);
    type_null    = pointer_type_create(type_void);
    type_uninit_value = builtin_type_create(TYPE_UNINIT,  str_lit("builtin(uninit)"), 0);

    type_bool  = builtin_type_create(TYPE_BOOL,     str_lit("bool"),   1);

    type_u8    = builtin_type_create(TYPE_UINT8,    str_lit("u8"),     1);
    type_u16   = builtin_type_create(TYPE_UINT16,   str_lit("u16"),    2);
    type_u32   = builtin_type_create(TYPE_UINT32,   str_lit("u32"),    4);
    type_u64   = builtin_type_create(TYPE_UINT64,   str_lit("u64"),    8);
    type_i8    = builtin_type_create(TYPE_INT8,     str_lit("i8"),     1);
    type_i16   = builtin_type_create(TYPE_INT16,    str_lit("i16"),    2);
    type_i32   = builtin_type_create(TYPE_INT32,    str_lit("i32"),    4);
    type_i64   = builtin_type_create(TYPE_INT64,    str_lit("i64"),    8);
    type_uint  = builtin_type_create(TYPE_UINT,     str_lit("uint"),   4);
    type_int   = builtin_type_create(TYPE_INT,      str_lit("int"),    4);

    type_usize = builtin_type_create(TYPE_USIZE,    str_lit("usize"),  8);
    type_isize = builtin_type_create(TYPE_ISIZE,    str_lit("isize"),  8);

    type_f32   = builtin_type_create(TYPE_FLOAT32,  str_lit("f32"),    4);
    type_f64   = builtin_type_create(TYPE_FLOAT64,  str_lit("f64"),    8);

    type_cstring = pointer_type_create(type_u8);
    type_string = builtin_type_create(TYPE_STRING, str_lit("string"), 16);

    type_any = builtin_type_create(TYPE_ANY, str_lit("any"), 16);
}

Type *type_untuple_maybe(Type *type) {
    if (is_tuple_type(type)) {
        Type_Tuple *tuple = (Type_Tuple *)type;
        if (tuple->types.count == 1) {
            return tuple->types[0];
        }
    }
    return type;
}

//@Todo More robust type checking for non-indirection types that are "aggregate" such as struct and procedure types.
//      For now we just check if they are identical, not equivalent.
bool is_convertible(Type *t0, Type *t1) {
    Assert(t0 != nullptr);
    Assert(t1 != nullptr);

    t0 = type_untuple_maybe(t0);
    t1 = type_untuple_maybe(t1);

    if (t0 == t1) return true;

    if (t0->kind == TYPE_UNINIT || t1->kind == TYPE_UNINIT) return true;

    //@Note Any results of poisoned types, just okay it
    // if (t0->is_poisoned || t1->is_poisoned) return true;

    //@Note Nullable types
    if (t1->kind == TYPE_NULL) {
        if (is_indirection_type(t0)) {
            return true;
        } else {
            return false;
        }
    }

    if (is_proc_type(t0) && is_proc_type(t1)) {
        Type_Proc *a = (Type_Proc *)t0;
        Type_Proc *b = (Type_Proc *)t1;
        if (a->params->types.count != b->params->types.count) {
            return false;
        }
        if (a->results->types.count != b->results->types.count) {
            return false;
        }

        for (int i = 0; i < a->params->types.count; i++) {
            if (!is_convertible(a->params->types[i], b->params->types[i])) {
                return false;
            }
        }
        for (int i = 0; i < a->results->types.count; i++) {
            if (!is_convertible(a->results->types[i], b->results->types[i])) {
                return false;
            }
        }
        return true;
    }

    //@Note Indirection testing
    if (is_indirection_type(t0) && is_integral_type(t1)) {
        return true;
    }

    if (is_indirection_type(t0) != is_indirection_type(t1)) {
        return false;
    }

    if (is_indirection_type(t0) && is_indirection_type(t1)) {
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
    if (is_enum_type(t0) || is_enum_type(t1)) {
        return false;
    }

    if (is_integral_type(t0) == is_integral_type(t1)) {
        if (t0->size == t1->size) {
            return true;
        }
    }
    return false;
}

bool typecheck_castable(Type *t0, Type *t1) {
    Assert(t0 && t1);

    if (t0 == t1) return true;

    if (is_numeric_type(t0) && is_numeric_type(t1)) {
        return true;
    } else if (is_integer_type(t0)) {
        return is_pointer_like_type(t1);
    } else if (is_integer_type(t1)) {
        return is_pointer_like_type(t0);
    } else if (is_pointer_like_type(t0) && is_pointer_like_type(t1)) {
        return true;
    } else {
        return false;
    }
}

Type *type_deref(Type *t) {
    if (t) {
        if (t->base) {
            return t->base;
        }
    }
    return t;
}

CString string_from_type(CString string, Type *type) {
    if (type == NULL)  {
        return string_append(string, "<notype>");
    }

    switch (type->kind) {
    default:
        string = string_append(string, (char *)type->name->data);
        break;

    case TYPE_ENUM: {
        Type_Enum *te = static_cast<Type_Enum*>(type);
        if (te->name) {
            string = string_append(string, te->name->data);
        } else {
            string = string_append(string, "<anon enum>");
        }
        break;
    }
        
    case TYPE_PROC: {
        Type_Proc *tp = static_cast<Type_Proc*>(type);
        string = string_from_type(string, tp->params);
        if (tp->results) {
            string = string_append(string, "->");
            string = string_from_type(string, tp->results);
        }
        break;
    }

    case TYPE_STRUCT: {
        Type_Struct *ts = static_cast<Type_Struct*>(type);
        if (ts->name) {
            string = string_append(string, ts->name->data);
        } else {
            string = string_append(string, "<anon struct>");
        }
        break;
    }

    case TYPE_UNION: {
        Type_Union *tu = static_cast<Type_Union*>(type);
        if (tu->name) {
            string = string_append(string, tu->name->data);
        } else {
            string = string_append(string, "<anon union>");
        }
        break;
    }

    case TYPE_POINTER: {
        Type_Pointer *tp = static_cast<Type_Pointer*>(type);
        string = string_append(string, "*");
        string = string_from_type(string, tp->base);
        break;
    }

    case TYPE_TUPLE: {
        Type_Tuple *tuple = static_cast<Type_Tuple*>(type);
        string = string_append(string, "(");
        for (int i = 0; i < tuple->types.count; i++) {
            Type *t = tuple->types[i];
            string = string_from_type(string, t);
            if (i < tuple->types.count-1) {
                string = string_append(string, ",");
            }
        }
        string = string_append(string, ")");
        break;
    }

    case TYPE_ARRAY: {
        Type_Array *ta = static_cast<Type_Array*>(type);
        string = string_append_fmt(string, "[%llu]", ta->array_size);
        string = string_from_type(string, ta->base);
        break;
    }

    case TYPE_ARRAY_VIEW: {
        Type_Array_View *tav = static_cast<Type_Array_View*>(type);
        string = string_append(string, "[]");
        string = string_from_type(string, tav->base);
        break;
    }

    case TYPE_DYNAMIC_ARRAY: {
        Type_Dynamic_Array *tda = static_cast<Type_Dynamic_Array*>(type);
        string = string_append(string, "[..]");
        string = string_from_type(string, tda->base);
        break;
    }
    }
    return string;
}

CString string_from_type(Allocator allocator, Type *type) {
    CString string = cstring_make(allocator, "");
    return string_from_type(string, type);
}

CString string_from_type(Type *type) {
    return string_from_type(heap_allocator(), type);
}
