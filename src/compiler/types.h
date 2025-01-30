#ifndef TYPES_H
#define TYPES_H

enum Type_Match_Result {
    TYPE_MATCH_EQUAL,
    TYPE_MATCH_ERROR_INDIRECTION,
};

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

#endif // TYPES_H
