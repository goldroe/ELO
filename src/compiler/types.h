#ifndef TYPES_H
#define TYPES_H

#include "array.h"

#include "base/base_strings.h"

struct Atom;
struct Decl;
struct Scope;
struct BE_Struct;

enum Type_Kind {
    TYPE_INVALID,

    TYPE_VOID,
    TYPE_NULL,

    TYPE_INTEGER_BEGIN,
    TYPE_UINT8,
    TYPE_UINT16,
    TYPE_UINT32,
    TYPE_UINT,
    TYPE_UINT64,

    TYPE_INT8,
    TYPE_INT16,
    TYPE_INT32,
    TYPE_INT,
    TYPE_INT64,

    TYPE_BOOL,
    TYPE_ISIZE,
    TYPE_USIZE,
    TYPE_INTEGER_END,

    TYPE_FLOAT32,
    TYPE_FLOAT64,

    TYPE_ENUM,
    TYPE_PROC,
    TYPE_STRUCT,
    TYPE_UNION,

    TYPE_POINTER,
    TYPE_TUPLE,
    TYPE_VARARGS,

    TYPE_ARRAY,
    TYPE_ARRAY_VIEW,
    TYPE_DYNAMIC_ARRAY,
    TYPE_STRING,

    TYPE_ANY
};

struct Type {
    Type *base = nullptr;
    Type_Kind kind = TYPE_INVALID;
    Decl *decl = nullptr;
    Atom *name = nullptr;
    s64 size = 0;
};

struct Type_Pointer : Type {
    Type_Pointer() { kind = TYPE_POINTER; }
};

struct Type_Array : Type {
    Type_Array() { kind = TYPE_ARRAY; }
    u64 array_size;
};

struct Type_Array_View : Type {
    Type_Array_View() { kind = TYPE_ARRAY_VIEW; }
};

struct Type_Dynamic_Array : Type {
    Type_Dynamic_Array() { kind = TYPE_DYNAMIC_ARRAY; }
};

struct Type_Tuple : Type {
    Type_Tuple() { kind = TYPE_TUPLE; }
    Array<Type*> types;
};

struct Type_Proc : Type {
    Type_Proc() { kind = TYPE_PROC; }
    Type_Tuple *params;
    Type_Tuple *results;
    int variadic_index = -1;
    b32 is_variadic = false;
    b32 is_foreign = false;
};

struct Type_Struct : Type {
    Type_Struct() { kind = TYPE_STRUCT; }
    Scope *scope;
    Array<Decl*> members;
    Array<s64> offsets;

    BE_Struct *backend_struct = nullptr;
};

struct Type_Union : Type {
    Type_Union() { kind = TYPE_UNION; }
    Scope *scope;
    Array<Decl*> members;
    Type *base_type = nullptr;

    void *backend_type = nullptr;
};

struct Type_Enum : Type {
    Type_Enum() { kind = TYPE_ENUM;}
    Scope *scope;
    Type *base_type;
    Array<Decl*> fields;
};

#define TYPE_ALLOC(T) (alloc_item(heap_allocator(), T))
#define TYPE_NEW(T) static_cast<T*>(&(*TYPE_ALLOC(T) = T()))

extern Array<Type*> g_builtin_types;
extern Type *type_invalid;
extern Type *type_void;
extern Type *type_null;
extern Type *type_bool;
extern Type *type_u8, *type_u16, *type_u32, *type_u64, *type_uint;
extern Type *type_i8, *type_i16, *type_i32, *type_i64, *type_int;
extern Type *type_isize, *type_usize;
extern Type *type_f32, *type_f64;
extern Type *type_string;
extern Type *type_cstring;
extern Type *type_any;

Type_Pointer *pointer_type_create(Type *elem);
Type_Array *array_type_create(Type *elem, u64 array_size);
Type_Array_View *array_view_type_create(Type *elem);
Type_Dynamic_Array *dynamic_array_type_create(Type *elem);
Type_Tuple *tuple_type_create(Array<Type*> types);
Type_Proc *proc_type_create(Array<Type*> params, Array<Type*> results);
Type_Struct *struct_type_create(Atom *name, Array<Decl*> members, Scope *scope);
Type_Union *union_type_create(Atom *name, Array<Decl*> members, Scope *scope);
Type_Enum *enum_type_create(Type *base_type, Array<Decl*> fields, Scope *scope);
Type *builtin_type_create(Type_Kind kind, String8 name, int size);

void register_builtin_types();

internal bool typecheck_castable(Type *t0, Type *t1);
bool is_convertible(Type *t0, Type *t1);

Type *type_deref(Type *t);

internal char *string_from_type(Type *ty);

internal inline bool is_tuple_type(Type *type)         {return type->kind == TYPE_TUPLE;}
internal inline bool is_array_type(Type *type)         {return type->kind == TYPE_ARRAY;}
internal inline bool is_array_view_type(Type *type)    {return type->kind == TYPE_ARRAY_VIEW;}
internal inline bool is_dynamic_array_type(Type *type) {return type->kind == TYPE_DYNAMIC_ARRAY;}
internal inline bool is_pointer_type(Type *type)       {return type->kind == TYPE_POINTER;}
internal inline bool is_boolean_type(Type *type)       {return type->kind == TYPE_BOOL;}
internal inline bool is_struct_type(Type *type)        {return type->kind == TYPE_STRUCT;}
internal inline bool is_union_type(Type *type)         {return type->kind == TYPE_UNION;}
internal inline bool is_enum_type(Type *type)          {return type->kind == TYPE_ENUM;}
internal inline bool is_proc_type(Type *type)          {return type->kind == TYPE_PROC;}

internal inline bool is_integer_type(Type *type) {
    return TYPE_INTEGER_BEGIN <= type->kind && type->kind <= TYPE_INTEGER_END;
}

internal inline bool is_float_type(Type *type) {
    return type->kind == TYPE_FLOAT32 || type->kind == TYPE_FLOAT64;
}

internal inline bool is_signed_type(Type *type) {
    return TYPE_INT8 <= type->kind && type->kind <= TYPE_INT64;
}
internal inline bool is_integral_type(Type *type) {return is_integer_type(type) || is_enum_type(type);}

internal inline bool is_user_defined_type(Type *type) {
    return type->kind == TYPE_STRUCT || type->kind == TYPE_ARRAY;
}

internal inline bool is_struct_or_pointer(Type *type) {
    return (is_struct_type(type) || (is_pointer_type(type) && is_struct_type(type->base)));
}

internal inline bool is_conditional_type(Type *type) {
    return is_integral_type(type) || is_enum_type(type) || is_boolean_type(type) || is_float_type(type) || is_pointer_type(type);
}

internal inline bool is_numeric_type(Type *type) {
    return is_integral_type(type) || is_enum_type(type) || is_boolean_type(type) || is_float_type(type) || is_pointer_type(type);
}

internal inline bool is_array_like_type(Type *type) {
    return is_array_type(type) || is_array_view_type(type) || is_dynamic_array_type(type);
}

internal inline bool is_indirection_type(Type *type) {
    return is_pointer_type(type) || is_array_like_type(type);
}

internal inline bool is_pointer_like_type(Type *type) {
    return is_pointer_type(type) || is_array_type(type) || is_proc_type(type);
}


#endif // TYPES_H
