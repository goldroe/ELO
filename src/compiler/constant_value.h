#if !defined(CONSTANT_VALUE_H)
#define CONSTANT_VALUE_H

#include <libtommath/tommath.h>
#include "base/base_strings.h"

#include "OP.h"

typedef mp_int bigint;
struct Type;

enum Constant_Value_Kind {
    CONSTANT_VALUE_INVALID,
    CONSTANT_VALUE_INTEGER,
    CONSTANT_VALUE_FLOAT,
    CONSTANT_VALUE_STRING,
    CONSTANT_VALUE_TYPEID,
};

struct Constant_Value {
    Constant_Value_Kind kind;
    union {
        bigint value_integer;
        f64 value_float;
        String value_string;
        Type *value_typeid;
    };
};

String string_from_bigint(bigint a);

i64 s64_from_bigint(bigint i);
i64 u64_from_bigint(bigint i);
f64 f64_from_bigint(bigint i);

bigint bigint_copy(const bigint *a);
bigint bigint_make(int value);
bigint bigint_u32_make(uint32_t value);
bigint bigint_i32_make(int32_t value);
bigint bigint_u64_make(uint64_t value);
bigint bigint_i64_make(int64_t value);
bigint bigint_f64_make(f64 f);

void bigint_add(bigint *dst, const bigint *a, const bigint *b);
void bigint_add(bigint *dst, const bigint *a, int d);
void bigint_sub(bigint *dst, const bigint *a, const bigint *b);
void bigint_mul(bigint *dst, const bigint *a, const bigint *b);
void bigint_div(bigint *dst, const bigint *a, const bigint *b);

void bigint_or(bigint *dst, const bigint *a, const bigint *b);
void bigint_xor(bigint *dst, const bigint *a, const bigint *b);
void bigint_and(bigint *dst, const bigint *a, const bigint *b);
void bigint_lazy_or(bigint *dst, const bigint *a, const bigint *b);
void bigint_lazy_and(bigint *dst, const bigint *a, const bigint *b);
void bigint_mod(bigint *dst, const bigint *a, const bigint *b);
void bigint_cmp(bigint *dst, const bigint *a, const bigint *b, OP op);

Constant_Value constant_value_int_make(bigint i);
Constant_Value constant_value_float_make(f64 f);
Constant_Value constant_value_string_make(String string);
Constant_Value constant_value_typeid_make(Type *type);
Constant_Value constant_cast_value(Constant_Value value, Type *ct);
Constant_Value constant_unary_op_value(OP op, Constant_Value x);
Constant_Value constant_binary_op_value(OP op, Constant_Value x, Constant_Value y);


#endif //CONSTANT_VALUE_H
