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

    inline bool is_integer_type() {
        return TYPE_INTEGER_BEGIN <= kind && kind <= TYPE_INTEGER_END;
    }

    inline bool is_float_type() {
        return kind == TYPE_FLOAT32 || kind == TYPE_FLOAT64;
    }

    inline bool is_integral_type() {
        return is_integer_type() || is_enum_type();
    }

    inline bool is_signed() {
        return TYPE_INT8 <= kind && kind <= TYPE_INT64;
    }

    inline bool is_array_type() {
        return kind == TYPE_ARRAY;
    }
    inline bool is_pointer_type() {
        return kind == TYPE_POINTER;
    }
    inline bool is_user_defined_type() {
        return kind == TYPE_STRUCT || kind == TYPE_ARRAY;
    }
    inline bool is_boolean_type() {
        return kind == TYPE_BOOL;
    }
    inline bool is_struct_type() {
        return kind == TYPE_STRUCT;
    }
    inline bool is_enum_type() {
        return kind == TYPE_ENUM;
    }
    inline bool is_proc_type() {
        return kind == TYPE_PROC;
    }

    inline bool is_struct_or_pointer() {
        return (this->is_struct_type() || (is_pointer_type() && this->base->is_struct_type()));
    }

    bool is_conditional_type() {
        return is_integral_type() || is_enum_type() || is_boolean_type() || is_float_type() || is_pointer_type();
    }

    bool is_numeric_type() {
        return is_integral_type() || is_enum_type() || is_boolean_type() || is_float_type() || is_pointer_type();
    }

    inline bool is_indirection_type() {
        return is_pointer_type() || is_array_type();
    }

    inline bool is_pointer_like_type() {
        return is_pointer_type() || is_array_type() || is_proc_type();
    }
};

struct Pointer_Type : Type {
    Pointer_Type() { kind = TYPE_POINTER; }
};

struct Array_Type : Type {
    Array_Type() {kind = TYPE_ARRAY;}
    b32 is_dynamic;
    b32 is_fixed;
    u64 array_size;
};

struct Tuple_Type : Type {
    Tuple_Type() { kind = TYPE_TUPLE; }
    Auto_Array<Type*> types;
};

struct Proc_Type : Type {
    Proc_Type() { kind = TYPE_PROC; }
    Tuple_Type *params;
    Tuple_Type *results;
    b32 variadic = false;
};

struct Struct_Field_Info {
    Atom *name;
    Type *type;
};

struct Struct_Type : Type {
    Struct_Type() { kind = TYPE_STRUCT; }
    Scope *scope;
    Auto_Array<Decl*> members;
};

struct Union_Type : Type {
    Union_Type() { kind = TYPE_UNION; }
    Scope *scope;
    Auto_Array<Decl*> members;
};

struct Enum_Type : Type {
    Enum_Type() { kind = TYPE_ENUM;}
    Scope *scope;
    Type *base_type;
    Auto_Array<Decl*> fields;
};

#define TYPE_ALLOC(T) (alloc_item(heap_allocator(), T))
#define TYPE_NEW(T) static_cast<T*>(&(*TYPE_ALLOC(T) = T()))

#endif // TYPES_H
