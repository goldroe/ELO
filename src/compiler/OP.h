#if !defined(OP_H)
#define OP_H

enum OP {
    OP_ERR = -1,

    OP_ADDRESS,
    OP_DEREF,
    OP_SUBSCRIPT,
    OP_SELECT,
    OP_CAST,

    // Unary
    OP_UNARY_PLUS,
    OP_UNARY_MINUS,
    OP_NOT,
    OP_BIT_NOT,

    OP_SIZEOF,

    // Binary
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,

    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_LTEQ,
    OP_GT,
    OP_GTEQ,

    OP_OR,
    OP_AND,
    OP_BIT_AND,
    OP_BIT_OR,
    OP_XOR,
    OP_LSH,
    OP_RSH,

    // Assign
    OP_ASSIGN,
    OP_IN,
    OP_ADD_ASSIGN,
    OP_SUB_ASSIGN,
    OP_MUL_ASSIGN,
    OP_DIV_ASSIGN,
    OP_MOD_ASSIGN,
    OP_LSHIFT_ASSIGN,
    OP_RSHIFT_ASSIGN,
    OP_AND_ASSIGN,
    OP_OR_ASSIGN,
    OP_XOR_ASSIGN,
    OP_LSH_ASSIGN,
    OP_RSH_ASSIGN,
    OP_ASSIGN_END,
};

#endif //OP_H
