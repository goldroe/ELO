#ifndef TYPES_H
#define TYPES_H

enum Builtin_Type_Kind {
    BUILTIN_TYPE_POISON,
    BUILTIN_TYPE_VOID,
    BUILTIN_TYPE_NULL,
    BUILTIN_TYPE_U8,
    BUILTIN_TYPE_U16,
    BUILTIN_TYPE_U32,
    BUILTIN_TYPE_U64,
    BUILTIN_TYPE_I8,
    BUILTIN_TYPE_I16,
    BUILTIN_TYPE_I32,
    BUILTIN_TYPE_I64,
    BUILTIN_TYPE_INT,
    BUILTIN_TYPE_UINT,
    BUILTIN_TYPE_ISIZE,
    BUILTIN_TYPE_USIZE,
    BUILTIN_TYPE_BOOL,
    BUILTIN_TYPE_F32,
    BUILTIN_TYPE_F64,
    BUILTIN_TYPE_STRING,
    BUILTIN_TYPE_COUNT
};

enum Type_Flags {
    TYPE_FLAG_VOID      = (1<<0),
    TYPE_FLAG_NULL      = (1<<1),
    TYPE_FLAG_BUILTIN   = (1<<2),
    TYPE_FLAG_POISON    = (1<<3),
    TYPE_FLAG_STRUCT    = (1<<4),
    TYPE_FLAG_ENUM      = (1<<5),
    TYPE_FLAG_PROC      = (1<<6),
    TYPE_FLAG_ARRAY     = (1<<7),
    TYPE_FLAG_POINTER   = (1<<8),
    TYPE_FLAG_INTEGER   = (1<<9),
    TYPE_FLAG_SIGNED    = (1<<10),
    TYPE_FLAG_FLOAT     = (1<<11),
    TYPE_FLAG_BOOLEAN   = (1<<12),
    TYPE_FLAG_STRING    = (1<<13),
    TYPE_FLAG_VARARGS   = (1<<14),

    TYPE_FLAG_INTEGRAL  = (TYPE_FLAG_INTEGER|TYPE_FLAG_BOOLEAN|TYPE_FLAG_ENUM),
    TYPE_FLAG_NUMERIC   = (TYPE_FLAG_INTEGER|TYPE_FLAG_BOOLEAN|TYPE_FLAG_ENUM|TYPE_FLAG_FLOAT|TYPE_FLAG_BOOLEAN|TYPE_FLAG_POINTER),
    TYPE_FLAG_AGGREGATE = (TYPE_FLAG_STRUCT|TYPE_FLAG_ARRAY|TYPE_FLAG_STRING),
};
EnumDefineFlagOperators(Type_Flags);

struct Struct_Field_Info {
    Atom *name;
    Type *type;
    int mem_offset;
};

struct Type : Ast {
    Type() { kind = AST_TYPE; }
    Type *base;
    Type_Flags type_flags;
    Ast_Decl *decl;
    Atom *name;
    int bytes;
    Builtin_Type_Kind builtin_kind;

    struct {
        Auto_Array<Struct_Field_Info> fields;
    } aggregate;

    bool is_struct_access();
    inline bool is_user_defined_type() { return type_flags & (TYPE_FLAG_STRUCT|TYPE_FLAG_ARRAY); }
    inline bool is_boolean_type() { return (type_flags & TYPE_FLAG_BOOLEAN); }
    inline bool is_custom_type() { return type_flags & (TYPE_FLAG_STRUCT|TYPE_FLAG_ENUM); }
    inline bool is_struct_type() { return type_flags & TYPE_FLAG_STRUCT; }
    inline bool is_enum_type() { return type_flags & TYPE_FLAG_ENUM; }
    inline bool is_proc_type() { return type_flags & TYPE_FLAG_PROC; }
    inline bool is_builtin_type() { return type_flags & TYPE_FLAG_BUILTIN; }
    inline bool is_conditional_type() { return type_flags & TYPE_FLAG_NUMERIC; }
    inline bool is_numeric_type() { return type_flags & TYPE_FLAG_NUMERIC; }
    inline bool is_array_type() { return type_flags & TYPE_FLAG_ARRAY; }
    inline bool is_pointer_type() { return type_flags & TYPE_FLAG_POINTER; }
    inline bool is_indirection_type() { return type_flags & (TYPE_FLAG_POINTER|TYPE_FLAG_ARRAY); }
    inline bool is_arithmetic_type() { return type_flags & TYPE_FLAG_NUMERIC; }
    inline bool is_integral_type() { return type_flags & TYPE_FLAG_INTEGRAL; }
    inline bool is_float_type() { return type_flags & TYPE_FLAG_FLOAT; }
    inline bool is_signed() { return type_flags & TYPE_FLAG_SIGNED; }
    inline Type *deref() { return base; }
};

struct Enum_Field_Info {
    Atom *name;
    s64 value;
};

struct Enum_Type : Type {
    Enum_Type() { kind = AST_ENUM_TYPE; }
    Auto_Array<Enum_Field_Info> fields;
};

struct Proc_Type : Type {
    Proc_Type() { kind = AST_PROC_TYPE; }
    Auto_Array<Type*> parameters;
    Type *return_type;
    b32 has_varargs;
}; 

struct Array_Type : Type {
    Array_Type() { kind = AST_ARRAY_TYPE; }
    b32 is_dynamic;
    b32 is_fixed;
    u64 array_size;
};

internal Type *pointer_type(Type *base);

#endif // TYPES_H
