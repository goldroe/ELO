#ifndef JB_H
#define JB_H

enum JB_OP {
    OP_NIL,

    OP_MOV,
    OP_PUSH,
    OP_POP,

    OP_INC,
    OP_DEC,
    OP_NEG,
    OP_NOT,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_XOR,
    OP_OR,
    OP_AND,

    OP_CMP_Z,
    OP_CMP_EQ,
    OP_CMP_GT,
    OP_CMP_GE,
    OP_CMP_LT,
    OP_CMP_LE,

    OP_JMP,
    OP_JMP_Z,
    OP_JMP_EQ,
    OP_JMP_GT,
    OP_JMP_GE,
    OP_JMP_LT,
    OP_JMP_LE,

    OP_CALL,
};

enum JB_Value_Kind {
    JB_VALUE_NIL,

    JB_VALUE_CONST_INT,
    JB_VALUE_CONST_FLOAT,
    JB_VALUE_CONST_STRING,
};

struct JB_Value {
    JB_Value_Kind kind;
    Ast_Type *type;
};

struct JB_Int {
    u64 val;
    int precision;
    bool is_signed;
};

struct JB_Float {
    f64 val;
    int precision;
};

struct JB_Constant : JB_Value {
    JB_Constant() { kind = JB_VALUE_CONST_INT };
    union {
        JB_Int int_val;
        JB_Float float_val;
        String8 string_val;
    }:
};

#endif // JB_H
