#ifndef TYPES_H
#define TYPES_H

enum Type_Kind {
    TYPE_INVALID,
    TYPE_VOID,
    TYPE_NULL,
    TYPE_POISON,

    TYPE_INTEGER_BEGIN,
    TYPE_UINT8,
    TYPE_UINT16,
    TYPE_UINT32,
    TYPE_UINT64,

    TYPE_INT8,
    TYPE_INT16,
    TYPE_INT32,
    TYPE_INT64,

    TYPE_UINT,
    TYPE_INT,

    TYPE_BOOL,
    TYPE_ISIZE,
    TYPE_USIZE,
    TYPE_INTEGER_END,

    TYPE_FLOAT32,
    TYPE_FLOAT64,

    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_TUPLE,
    TYPE_VARARGS,

    TYPE_ENUM,
    TYPE_PROC,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_STRING,
};

enum Type_Flags {
    TYPE_FLAG_INTEGRAL  = (1<<0),
    TYPE_FLAG_SIGNED    = (1<<1),
    TYPE_FLAG_FLOAT     = (1<<2),
//     TYPE_FLAG_INTEGRAL  = (TYPE_FLAG_INTEGER|TYPE_FLAG_BOOLEAN|TYPE_FLAG_ENUM),
//     TYPE_FLAG_NUMERIC   = (TYPE_FLAG_INTEGER|TYPE_FLAG_BOOLEAN|TYPE_FLAG_ENUM|TYPE_FLAG_FLOAT|TYPE_FLAG_BOOLEAN|TYPE_FLAG_POINTER),
//     TYPE_FLAG_AGGREGATE = (TYPE_FLAG_STRUCT|TYPE_FLAG_ARRAY|TYPE_FLAG_STRING),
};
EnumDefineFlagOperators(Type_Flags);

struct Type {
    Type *base = nullptr;
    Type_Kind kind = TYPE_INVALID;
    Type_Flags type_flags = (Type_Flags)0;
    Decl *decl = nullptr;
    Atom *name = nullptr;

    int bytes = 0;

};

struct Type_Pointer : Type {
    Type_Pointer() { kind = TYPE_POINTER; }
};

struct Type_Array : Type {
    Type_Array() {kind = TYPE_ARRAY;}
    b32 is_dynamic;
    b32 is_fixed;
    u64 array_size;
};

struct Type_Tuple : Type {
    Type_Tuple() { kind = TYPE_TUPLE; }
    Auto_Array<Type*> types;
};

struct Type_Proc : Type {
    Type_Proc() { kind = TYPE_PROC; }
    Type_Tuple *params;
    Type_Tuple *results;
    b32 variadic = false;
    b32 foreign = false;
};

struct Type_Struct : Type {
    Type_Struct() { kind = TYPE_STRUCT; }
    Scope *scope;
    Auto_Array<Decl*> members;

    BE_Struct *backend_struct = nullptr;
};

struct Type_Union : Type {
    Type_Union() { kind = TYPE_UNION; }
    Scope *scope;
    Auto_Array<Decl*> members;
};

struct Type_Enum : Type {
    Type_Enum() { kind = TYPE_ENUM;}
    Scope *scope;
    Type *base_type;
    Auto_Array<Decl*> fields;
};

#define TYPE_ALLOC(T) (alloc_item(heap_allocator(), T))
#define TYPE_NEW(T) static_cast<T*>(&(*TYPE_ALLOC(T) = T()))

internal inline bool is_integer_type(Type *type) {
    return TYPE_INTEGER_BEGIN <= type->kind && type->kind <= TYPE_INTEGER_END;
}

internal inline bool is_float_type(Type *type) {
    return type->kind == TYPE_FLOAT32 || type->kind == TYPE_FLOAT64;
}

internal inline bool is_boolean_type(Type *type) {
    return type->kind == TYPE_BOOL;
}

internal inline bool is_struct_type(Type *type) {
    return type->kind == TYPE_STRUCT;
}

internal inline bool is_enum_type(Type *type) {
    return type->kind == TYPE_ENUM;
}

internal inline bool is_proc_type(Type *type) {
    return type->kind == TYPE_PROC;
}

internal inline bool is_tuple_type(Type *type) {
    return type->kind == TYPE_TUPLE;
}

internal inline bool is_integral_type(Type *type) {
    return is_integer_type(type) || is_enum_type(type);
}

internal inline bool is_signed_type(Type *type) {
    return TYPE_INT8 <= type->kind && type->kind <= TYPE_INT64;
}

internal inline bool is_array_type(Type *type) {
    return type->kind == TYPE_ARRAY;
}

internal inline bool is_pointer_type(Type *type) {
    return type->kind == TYPE_POINTER;
}

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

internal inline bool is_indirection_type(Type *type) {
    return is_pointer_type(type) || is_array_type(type);
}

internal inline bool is_pointer_like_type(Type *type) {
    return is_pointer_type(type) || is_array_type(type) || is_proc_type(type);
}

#endif // TYPES_H
