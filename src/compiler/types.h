#ifndef TYPES_H
#define TYPES_H

enum Builtin_Type_Kind {
    BUILTIN_TYPE_POISON,
    BUILTIN_TYPE_VOID,
    BUILTIN_TYPE_U8,
    BUILTIN_TYPE_U16,
    BUILTIN_TYPE_U32,
    BUILTIN_TYPE_U64,
    BUILTIN_TYPE_S8,
    BUILTIN_TYPE_S16,
    BUILTIN_TYPE_S32,
    BUILTIN_TYPE_S64,
    BUILTIN_TYPE_BOOL,
    BUILTIN_TYPE_F32,
    BUILTIN_TYPE_F64,
    BUILTIN_TYPE_COUNT
};

enum Type_Info_Flags {
    TYPE_FLAG_NIL       = 0,
    TYPE_FLAG_BUILTIN   = (1<<0),
    TYPE_FLAG_POISON    = (1<<1),
    TYPE_FLAG_STRUCT    = (1<<2),
    TYPE_FLAG_ENUM      = (1<<3),
    TYPE_FLAG_PROC      = (1<<4),
    TYPE_FLAG_ARRAY     = (1<<5),
    TYPE_FLAG_POINTER   = (1<<6),
    TYPE_FLAG_INTEGER   = (1<<7),
    TYPE_FLAG_SIGNED    = (1<<8),
    TYPE_FLAG_FLOAT     = (1<<9),
    TYPE_FLAG_BOOLEAN   = (1<<10),
    TYPE_FLAG_INTEGRAL  = (TYPE_FLAG_INTEGER|TYPE_FLAG_BOOLEAN|TYPE_FLAG_ENUM),
    TYPE_FLAG_NUMERIC   = (TYPE_FLAG_INTEGER|TYPE_FLAG_FLOAT|TYPE_FLAG_BOOLEAN),
};
EnumDefineFlagOperators(Type_Info_Flags);

struct Ast_Type_Info : Ast {
    Ast_Type_Info() { kind = AST_TYPE_INFO; }
    Ast_Type_Info *base;
    Type_Info_Flags type_flags;
    Ast_Decl *decl;
    Atom *name;
    int bytes;
    Builtin_Type_Kind builtin_kind;

    inline bool is_custom_type() { return (type_flags & TYPE_FLAG_STRUCT) || (type_flags & TYPE_FLAG_ENUM); }
    inline bool is_struct_type() { return type_flags & TYPE_FLAG_STRUCT; }
    inline bool is_enum_type() { return type_flags & TYPE_FLAG_ENUM; }
    inline bool is_proc_type() { return type_flags & TYPE_FLAG_PROC; }
    inline bool is_builtin_type() { return type_flags & TYPE_FLAG_BUILTIN; }
    inline bool is_conditional_type() { return (type_flags & TYPE_FLAG_NUMERIC) || (type_flags & TYPE_FLAG_POINTER) || (type_flags & TYPE_FLAG_ENUM); }
    inline bool is_array_type() { return type_flags & TYPE_FLAG_ARRAY; }
    inline bool is_pointer_type() { return type_flags & TYPE_FLAG_POINTER; }
    inline bool is_indirection_type() { return (type_flags & TYPE_FLAG_POINTER) || (type_flags & TYPE_FLAG_ARRAY); }
    inline bool is_arithmetic_type() { return (type_flags & TYPE_FLAG_POINTER) || (type_flags & TYPE_FLAG_NUMERIC) || (type_flags & TYPE_FLAG_ENUM); }
    inline bool is_integral_type() { return (type_flags & TYPE_FLAG_INTEGRAL); }
    inline bool is_float_type() { return (type_flags & TYPE_FLAG_FLOAT); }
    inline bool is_signed() { return (type_flags & TYPE_FLAG_SIGNED); }
    inline Ast_Type_Info *deref() { return base; }
};

struct Struct_Field_Info {
    Ast_Type_Info *type;
    int mem_offset;
};

struct Ast_Struct_Type_Info : Ast_Type_Info {
    Ast_Struct_Type_Info() { kind = AST_STRUCT_TYPE_INFO; }
    Auto_Array<Struct_Field_Info> fields;
    u64 mem_bytes;
};

struct Enum_Field_Info {
    Atom *name;
};

struct Ast_Enum_Type_Info : Ast_Type_Info {
    Ast_Enum_Type_Info() { kind = AST_ENUM_TYPE_INFO; }
    Auto_Array<Enum_Field_Info> fields;
};

struct Ast_Proc_Type_Info : Ast_Type_Info {
    Ast_Proc_Type_Info() { kind = AST_PROC_TYPE_INFO; }
    Auto_Array<Ast_Type_Info*> parameters;
    Ast_Type_Info *return_type;
}; 

struct Ast_Array_Type_Info : Ast_Type_Info {
    Ast_Array_Type_Info() { kind = AST_ARRAY_TYPE_INFO; }
    b32 is_dynamic;
    b32 is_fixed;
    u64 array_size;
};

#endif // TYPES_H
