
internal u64 u64_from_bigint(bigint i) {
    u64 u = mp_get_u64(&i);
    return u;
}

internal f64 f64_from_bigint(bigint i) {
    f64 f = mp_get_double(&i);
    return f;
}

internal bigint bigint_make(int value) {
    bigint i = {};
    mp_init_set(&i, value);
    return i;
}

internal bigint bigint_make(u64 value) {
    bigint i = {};
    mp_init_u64(&i, value);
    return i;
}

internal bigint bigint_from_f64(f64 f) {
    bigint i = {};
    mp_init(&i);
    u64 u = (u64)f;
    mp_set_u64(&i, u);
    return i;
}

internal void bigint_add(bigint *dst, const bigint *a, const bigint *b) {
    mp_add(a, b, dst);
}

internal void bigint_add(bigint *dst, const bigint *a, int d) {
    mp_add_d(a, d, dst);
}

internal void bigint_sub(bigint *dst, const bigint *a, const bigint *b) {
    mp_sub(a, b, dst);
}

internal void bigint_mul(bigint *dst, const bigint *a, const bigint *b) {
    mp_mul(a, b, dst);
}

internal void bigint_div(bigint *dst, const bigint *a, const bigint *b) {
    bigint r = {};
    mp_div(a, b, dst, &r);
}

internal void bigint_or(bigint *dst, const bigint *a, const bigint *b) {
    mp_or(a, b, dst);
}

internal void bigint_xor(bigint *dst, const bigint *a, const bigint *b) {
    mp_xor(a, b, dst);
}

internal void bigint_and(bigint *dst, const bigint *a, const bigint *b) {
    mp_and(a, b, dst);
}

internal String string_from_bigint(bigint a) {
    local_persist char buffer[128];
    size_t written = 0;
    int err = mp_to_radix(&a, buffer, 128, &written, 10);
    String str = str8_copy(heap_allocator(), str8((u8 *)buffer, 128));
    return str;
}

internal void bigint_lazy_or(bigint *dst, const bigint *a, const bigint *b) {
    if (mp_iszero(a) && mp_iszero(b)) {
        mp_set(dst, 0);
    } else {
        mp_set(dst, 1);
    }
}

internal void bigint_mod(bigint *dst, const bigint *a, const bigint *b) {
    mp_mod(a, b, dst);
}

internal void bigint_lazy_and(bigint *dst, const bigint *a, const bigint *b) {
    if (mp_iszero(a) || mp_iszero(b)) {
        mp_set(dst, 0);
    } else {
        mp_set(dst, 1);
    }
}

internal void bigint_cmp(bigint *dst, const bigint *a, const bigint *b, OP op) {
    mp_ord ord = mp_cmp(a, b);
    mp_digit x = 0;
    switch (op) {
    case OP_EQ:   x = ord==MP_EQ; break;
    case OP_NEQ:  x = ord!=MP_EQ; break;
    case OP_LT:   x = ord==MP_LT; break;
    case OP_GT:   x = ord==MP_GT; break;
    case OP_LTEQ: x = (ord==MP_LT)|(ord==MP_EQ); break;
    case OP_GTEQ: x = (ord==MP_GT)|(ord==MP_EQ); break;
    }
    mp_init_set(dst, x);
}

internal Constant_Value make_constant_value_int(bigint value) {
    Constant_Value result = {};
    result.kind = CONSTANT_VALUE_INTEGER;
    result.value_integer = value;
    return result;
}

internal Constant_Value make_constant_value_float(f64 value) {
    Constant_Value result = {};
    result.kind = CONSTANT_VALUE_FLOAT;
    result.value_float = value;
    return result;
}

internal Constant_Value constant_cast_value(Constant_Value value, Type *ct) {
    switch (value.kind) {
    case CONSTANT_VALUE_INTEGER: {
        if (ct->is_float_type()) {
            return make_constant_value_float(f64_from_bigint(value.value_integer));
        }
        break;
    }

    case CONSTANT_VALUE_FLOAT: {
        if (ct->is_integral_type()) {
            return make_constant_value_int(bigint_from_f64(value.value_float));
        }
        break;
    }
    }
    return value;
}

internal Constant_Value constant_unary_op_value(OP op, Constant_Value x) {
    switch (x.kind) {
    case CONSTANT_VALUE_INTEGER: {
        Constant_Value v = {CONSTANT_VALUE_INTEGER};
        switch (op) {
        case OP_UNARY_PLUS: return x;
        case OP_UNARY_MINUS:
            mp_neg(&x.value_integer, &v.value_integer);
            return v;

        case OP_NOT:
            if (mp_iszero(&v.value_integer)) {
                mp_init_set(&v.value_integer, 1);
            } else {
                mp_init_set(&v.value_integer, 0);
            }
            return v;

        case OP_BIT_NOT:
            mp_complement(&x.value_integer, &v.value_integer);
            return v;
        }
        break;
    }

    case CONSTANT_VALUE_FLOAT: {
        f64 a = x.value_float;
        f64 b = 0.0;
        switch (op) {
        case OP_UNARY_PLUS: return x;
        case OP_UNARY_MINUS: b = - a; break;
        case OP_NOT: b = !a; break;
        }
        return make_constant_value_float(b);
    }
    }

    return {};
}

internal Constant_Value constant_binary_op_value(OP op, Constant_Value x, Constant_Value y) {
    switch (x.kind) {
    case CONSTANT_VALUE_INTEGER: {
        bigint *a = &x.value_integer;
        bigint *b = &y.value_integer;
        bigint c = {};
        switch (op) {
        case OP_ADD: bigint_add(&c, a, b); break;
        case OP_SUB: bigint_sub(&c, a, b); break;
        case OP_MUL: bigint_mul(&c, a, b); break;
        case OP_DIV: bigint_div(&c, a, b); break;
        case OP_MOD: bigint_mod(&c, a, b); break;
        case OP_XOR: bigint_xor(&c, a, b); break;
        case OP_BIT_OR:  bigint_or(&c, a, b); break;
        case OP_BIT_AND: bigint_and(&c, a, b); break;
        case OP_OR:  bigint_lazy_or(&c, a, b); break;
        case OP_AND: bigint_lazy_and(&c, a, b); break;

        case OP_EQ:
        case OP_NEQ:
        case OP_LT:
        case OP_GT:
        case OP_LTEQ:
        case OP_GTEQ:
            bigint_cmp(&c, a, b, op);
            break;
        }
        return make_constant_value_int(c);
    }

    case CONSTANT_VALUE_FLOAT: {
        f64 a = x.value_float;
        f64 b = y.value_float;
        f64 c = 0.0;
        switch (op) {
        case OP_ADD: c = a + b; break;
        case OP_SUB: c = a - b; break;
        case OP_MUL: c = a * b; break;
        case OP_DIV: c = a / b; break;
        case OP_MOD: c = fmod(a, b); break;
        case OP_EQ:   c = a == b; break;
        case OP_NEQ:  c = a != b; break;
        case OP_LT:   c = a < b;  break;
        case OP_LTEQ: c = a <= b; break;
        case OP_GT:   c = a > b;  break;
        case OP_GTEQ: c = a >= b; break;
        case OP_OR:   c = a || b; break;
        case OP_AND:  c = a && b; break;
        }
        return make_constant_value_float(c);
    }
    }

    return {};
}
