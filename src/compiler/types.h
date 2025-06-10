#ifndef TYPES_H
#define TYPES_H

enum Type_ID {
    TYPEID_POISON,
    TYPEID_VOID,
    TYPEID_NULL,

    TYPEID_INTEGER_BEGIN,
    TYPEID_UINT8,
    TYPEID_UINT16,
    TYPEID_UINT32,
    TYPEID_UINT64,

    TYPEID_INT8,
    TYPEID_INT16,
    TYPEID_INT32,
    TYPEID_INT64,

    TYPEID_UINT,
    TYPEID_INT,

    TYPEID_BOOL,
    TYPEID_ISIZE,
    TYPEID_USIZE,
    TYPEID_INTEGER_END,

    TYPEID_FLOAT32,
    TYPEID_FLOAT64,

    TYPEID_POINTER,

    TYPEID_STRUCT,
    TYPEID_PROC,
    TYPEID_ENUM,

    TYPEID_ARRAY,
    TYPEID_STRING,
    TYPEID_VARARGS,
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

struct Struct_Field_Info {
    Atom *name;
    Type *type;
};

struct Type : Ast {
    Type() { kind = AST_TYPE; }
    Type *base;
    Type_ID id;
    Type_Flags type_flags;
    Ast_Decl *decl;
    Atom *name;
    int bytes;

    struct {
        Auto_Array<Struct_Field_Info> fields;
    } aggregate;


    inline bool is_integer_type() {
        return TYPEID_INTEGER_BEGIN <= id && id <= TYPEID_INTEGER_END;
    }

    inline bool is_float_type() {
        return id == TYPEID_FLOAT32 || id == TYPEID_FLOAT64;
    }

    inline bool is_integral_type() {
        return is_integer_type() || is_enum_type();
    }

    inline bool is_signed() {
        return TYPEID_INT8 <= id && id <= TYPEID_INT64;
    }

    inline bool is_array_type() {
        return id == TYPEID_ARRAY;
    }
    inline bool is_pointer_type() {
        return id == TYPEID_POINTER;
    }
    inline bool is_user_defined_type() {
        return id == TYPEID_STRUCT || id == TYPEID_ARRAY;
    }
    inline bool is_boolean_type() {
        return id == TYPEID_BOOL;
    }
    inline bool is_struct_type() {
        return id == TYPEID_STRUCT;
    }
    inline bool is_enum_type() {
        return id == TYPEID_ENUM;
    }
    inline bool is_proc_type() {
        return id == TYPEID_PROC;
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

struct Enum_Field_Info {
    Atom *name;
    s64 value;
};

struct Enum_Type : Type {
    Enum_Type() {
        kind = AST_ENUM_TYPE;
        id = TYPEID_ENUM;
    }
    Auto_Array<Enum_Field_Info> fields;
};

struct Proc_Type : Type {
    Proc_Type() {
        kind = AST_PROC_TYPE;
        id = TYPEID_PROC;
    }
    Auto_Array<Type*> parameters;
    Type *return_type;
    b32 has_varargs;
}; 

struct Array_Type : Type {
    Array_Type() {
        kind = AST_ARRAY_TYPE;
        id = TYPEID_ARRAY;
    }
    b32 is_dynamic;
    b32 is_fixed;
    u64 array_size;
};

internal Type *pointer_type(Type *base);

#endif // TYPES_H
