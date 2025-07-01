#if !defined(CONSTANT_VALUE_H)
#define CONSTANT_VALUE_H

typedef mp_int bigint;

enum Constant_Value_Kind {
    CONSTANT_VALUE_INVALID,
    CONSTANT_VALUE_INTEGER,
    CONSTANT_VALUE_FLOAT,
    CONSTANT_VALUE_STRING,
};

struct Constant_Value {
    Constant_Value_Kind kind;
    union {
        bigint value_integer;
        f64 value_float;
        String value_string;
    };
};

#endif //CONSTANT_VALUE_H
