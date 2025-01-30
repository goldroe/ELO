inline internal V2_F32 v2_f32(f32 x, f32 y) {V2_F32 result = {x, y}; return result;}
inline internal V3_F32 v3_f32(f32 x, f32 y, f32 z) {V3_F32 result = {x, y, z}; return result;}
inline internal V4_F32 v4_f32(f32 x, f32 y, f32 z, f32 w) {V4_F32 result = {x, y, z, w}; return result;}

inline internal V2_F64 v2_f64(f64 x, f64 y) {V2_F64 result = {x, y}; return result;}
inline internal V3_F64 v3_f64(f64 x, f64 y, f32 z) {V3_F64 result = {x, y, z}; return result;}
inline internal V4_F64 v4_f64(f64 x, f64 y, f64 z, f64 w) {V4_F64 result = {x, y, z, w}; return result;}

inline internal V2_S32 v2_s32(s32 x, s32 y) {V2_S32 result = {x, y}; return result;}
inline internal V3_S32 v3_s32(s32 x, s32 y, s32 z) {V3_S32 result = {x, y, z}; return result;}
inline internal V4_S32 v4_s32(s32 x, s32 y, s32 z, s32 w) {V4_S32 result = {x, y, z, w}; return result;}

inline internal V2_S64 v2_s64(s64 x, s64 y) {V2_S64 result = {x, y}; return result;}
inline internal V3_S64 v3_s64(s64 x, s64 y, s64 z) {V3_S64 result = {x, y, z}; return result;}
inline internal V4_S64 v4_s64(s64 x, s64 y, s64 z, s64 w) {V4_S64 result = {x, y, z, w}; return result;}

inline internal V2_F32 v2_f32_from_v2_s32(V2_S32 v) {V2_F32 result; result.x = (f32)v.x; result.y = (f32)v.y; return result;}
inline internal V3_F32 v3_f32_from_v3_s32(V3_S32 v) {V3_F32 result; result.x = (f32)v.x; result.y = (f32)v.y; result.z = (f32)v.z; return result;}

inline internal V2_S32 v2_s32_from_v2_f32(V2_F32 v) {V2_S32 result; result.x = (s32)v.x; result.y = (s32)v.y; return result;}
inline internal V3_S32 v3_s32_from_v3_f32(V3_F32 v) {V3_S32 result; result.x = (s32)v.x; result.y = (s32)v.y; result.z = (s32)v.z; return result;}

inline internal V2_F32 fill_v2_f32(f32 v) {V2_F32 result; result.x = v; result.y = v; return result;}
inline internal V3_F32 fill_v3_f32(f32 v) {V3_F32 result; result.x = v; result.y = v; result.z = v; return result;}
inline internal V4_F32 fill_v4_f32(f32 v) {V4_F32 result; result.x = v; result.y = v; result.z = v; result.w = v; return result;}
inline internal V2_F64 fill_v2_f64(f64 v) {V2_F64 result; result.x = v; result.y = v; return result;}
inline internal V3_F64 fill_v3_f64(f64 v) {V3_F64 result; result.x = v; result.y = v; result.z = v; return result;}
inline internal V4_F64 fill_v4_f64(f64 v) {V4_F64 result; result.x = v; result.y = v; result.z = v; result.w = v; return result;}

inline internal M4_F32 m4_f32(f32 d) {
    M4_F32 result = {};
    result._00 = d;
    result._11 = d;
    result._22 = d;
    result._33 = d;
    return result;
}

inline internal M4_F32 transpose_m4_f32(M4_F32 m) {
    M4_F32 result = m;
    result._01 = m._10;
    result._02 = m._20;
    result._03 = m._30;
    result._10 = m._01;
    result._12 = m._21;
    result._13 = m._31;
    result._20 = m._02;
    result._21 = m._12;
    result._23 = m._32;
    result._30 = m._03;
    result._31 = m._13;
    result._32 = m._23;
    return result;
}

inline internal V4_F32 linear_combine_v4_m4_f32(V4_F32 left, M4_F32 right) {
    V4_F32 result;

    result.x = left.e[0] * right.columns[0].x;
    result.y = left.e[0] * right.columns[0].y;
    result.z = left.e[0] * right.columns[0].z;
    result.w = left.e[0] * right.columns[0].w;

    result.x += left.e[1] * right.columns[1].x;
    result.y += left.e[1] * right.columns[1].y;
    result.z += left.e[1] * right.columns[1].z;
    result.w += left.e[1] * right.columns[1].w;

    result.x += left.e[2] * right.columns[2].x;
    result.y += left.e[2] * right.columns[2].y;
    result.z += left.e[2] * right.columns[2].z;
    result.w += left.e[2] * right.columns[2].w;

    result.x += left.e[3] * right.columns[3].x;
    result.y += left.e[3] * right.columns[3].y;
    result.z += left.e[3] * right.columns[3].z;
    result.w += left.e[3] * right.columns[3].w;

    return result;
}

inline internal M4_F32 mul_m4_f32(M4_F32 left, M4_F32 right) {
    M4_F32 result;
    result.columns[0] = linear_combine_v4_m4_f32(right.columns[0], left);
    result.columns[1] = linear_combine_v4_m4_f32(right.columns[1], left);
    result.columns[2] = linear_combine_v4_m4_f32(right.columns[2], left);
    result.columns[3] = linear_combine_v4_m4_f32(right.columns[3], left);
    return result;
}

inline internal V4_F32 mul_m4_v4_f32(M4_F32 m, V4_F32 v) {
    V4_F32 result;
    result.e[0] = v.e[0] * m._00 + v.e[1] * m._01 + v.e[2] * m._02 + v.e[3] * m._03;
    result.e[1] = v.e[0] * m._10 + v.e[1] * m._11 + v.e[2] * m._12 + v.e[3] * m._13;
    result.e[2] = v.e[0] * m._20 + v.e[1] * m._21 + v.e[2] * m._22 + v.e[3] * m._23;
    result.e[3] = v.e[0] * m._30 + v.e[1] * m._31 + v.e[2] * m._32 + v.e[3] * m._33;
    return result;
}

inline internal M4_F32 translate_m4_f32(f32 x, f32 y, f32 z) {
    M4_F32 result = m4_f32(1.0f);
    result._30 = x;
    result._31 = y;
    result._32 = z;
    return result;
}

inline internal M4_F32 inv_translate_m4_f32(f32 x, f32 y, f32 z) {
    M4_F32 result = translate_m4_f32(-x, -y, -z);
    return result;
}

inline internal M4_F32 scale_m4_f32(f32 x, f32 y, f32 z) {
    M4_F32 result = {};
    result._00 = x;
    result._11 = y;
    result._22 = z;
    result._33 = 1.0f;
    return result;
}

inline internal M4_F32 rotate_rh_m4_f32(f32 angle, V3_F32 axis) {
    M4_F32 result = m4_f32(1.0f);
    axis = normalize_v3_f32(axis);

    f32 s = sinf(angle);
    f32 c = cosf(angle);
    f32 c_inv = 1.0f - c;

    result._00 = (c_inv * axis.x * axis.x) + c;
    result._01 = (c_inv * axis.x * axis.y) + (axis.z * s);
    result._02 = (c_inv * axis.x * axis.z) - (axis.y * s);

    result._10 = (c_inv * axis.y * axis.x) - (axis.z * s);
    result._11 = (c_inv * axis.y * axis.y) + c;
    result._12 = (c_inv * axis.y * axis.z) + (axis.x * s);
                                                        
    result._20 = (c_inv * axis.z * axis.x) + (axis.y * s);
    result._21 = (c_inv * axis.z * axis.y) - (axis.x * s);
    result._22 = (c_inv * axis.z * axis.z) + c;

    return result; 
}

inline internal M4_F32 ortho_rh_zo(f32 left, f32 right, f32 bottom, f32 top, f32 _near, f32 _far) {
    M4_F32 result = m4_f32(1.0f);
    result._00 = 2.0f / (right - left);
    result._11 = 2.0f / (top - bottom);
    result._22 = 1.0f / (_far - _near);
    result._30 = - (right + left) / (right - left);
    result._31 = - (top + bottom) / (top - bottom);
    // result._32 = - near / (far - near);
    result._32 = -(_near + _far) / (_far - _near);
    return result;
}

inline internal M4_F32 perspective_rh_zo(f32 fov, f32 aspect, f32 _near, f32 _far) {
    M4_F32 result = m4_f32(1.0f);
    f32 c = 1.0f / tanf(fov / 2.0f);
    result._00 = c / aspect;
    result._11 = c;
    result._23 = -1.0f;
    result._22 = (_far) / (_near - _far);
    result._32 = (_near * _far) / (_near - _far);
    return result;
}

inline internal M4_F32 look_at_rh_zo(V3_F32 eye, V3_F32 target, V3_F32 up) {
    V3_F32 F = normalize_v3_f32(sub_v3_f32(target, eye));
    V3_F32 R = normalize_v3_f32(cross_v3_f32(F, up));
    V3_F32 U = cross_v3_f32(R, F);

    M4_F32 result;
    result._00 = R.x;
    result._01 = U.x;
    result._02 = -F.x;
    result._03 = 0.f;
    result._10 = R.y;
    result._11 = U.y;
    result._12 = -F.y;
    result._13 = 0.f;
    result._20 = R.z;
    result._21 = U.z;
    result._22 = -F.z;
    result._23 = 0.f;
    result._30 = -dot_v3_f32(R, eye);
    result._31 = -dot_v3_f32(U, eye);
    result._32 =  dot_v3_f32(F, eye);
    result._33 = 1.f;
    return result;
}

inline internal M4_F32 look_at_lh_zo(V3_F32 eye, V3_F32 target, V3_F32 up) {
    V3_F32 F = normalize_v3_f32(sub_v3_f32(target, eye));
    V3_F32 R = normalize_v3_f32(cross_v3_f32(up, F));
    V3_F32 U = cross_v3_f32(F, R);

    M4_F32 result = m4_f32(1.0f);
    result._00 = R.x;
    result._10 = R.y;
    result._20 = R.z;
    result._01 = U.x;
    result._11 = U.y;
    result._21 = U.z;
    result._02 = F.x;
    result._12 = F.y;
    result._22 = F.z;
    result._30 = -dot_v3_f32(R, eye);
    result._31 = -dot_v3_f32(U, eye);
    result._32 = -dot_v3_f32(F, eye);
    return result;
}

inline internal f32 length_v2_f32(V2_F32 v) {
    f32 result;
    result = sqrtf(v.x * v.x + v.y * v.y);
    return result;
}

inline internal f32 length_v3_f32(V3_F32 v) {
    f32 result;
    result = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    return result;
}

inline internal f32 length_v4_f32(V4_F32 v) {
    f32 result;
    result = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    return result;
}

inline internal f32 length2_v2_f32(V2_F32 v) {
    f32 result;
    result = v.x * v.x + v.y * v.y;
    return result;
}

inline internal f32 length2_v3_f32(V3_F32 v) {
    f32 result = v.x * v.x + v.y * v.y + v.z * v.z;
    return result;
}

inline internal f32 length2_v4_f32(V4_F32 v) {
    f32 result = v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
    return result;
}

inline internal f32 angle_from_v2_f32(V2_F32 v) {
    f32 result = atan2f(v.y, v.x);
    return result;
}

inline internal V2_F32 direction_from_angle(f32 angle) {
    V2_F32 result;
    result.x = cosf(angle);
    result.y = sinf(angle);
    return result;
}

inline internal V2_F32 normalize_v2_f32(V2_F32 v) {
    V2_F32 result;
    f32 length = length_v2_f32(v);
    result = (length == 0.f) ? v2_f32(0.0f, 0.0f) : div_v2_f32_f(v, length);
    return result;
}

inline internal V3_F32 normalize_v3_f32(V3_F32 v) {
    V3_F32 result;
    f32 length = length_v3_f32(v);
    result = (length == 0.f) ? v3_f32(0.0f, 0.0f, 0.0f) : div_v3_f32_f(v, length);
    return result;
}

inline internal V4_F32 normalize_v4_f32(V4_F32 v) {
    V4_F32 result;
    f32 length = length_v4_f32(v);
    result = (length == 0.f) ? v4_f32(0.0f, 0.0f, 0.0f, 0.0f) : div_v4_f32_f(v, length);
    return result;
}

inline internal f32 dot_v2_f32(V2_F32 a, V2_F32 b) {
    f32 result = a.x * b.x + a.y * b.y;
    return result;
}

inline internal f32 dot_v3_f32(V3_F32 a, V3_F32 b) {
    f32 result;
    result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result; 
}

inline internal f32 dot_v4_f32(V4_F32 a, V4_F32 b) {
    f32 result;
    result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return result; 
}

inline internal V3_F32 cross_v3_f32(V3_F32 a, V3_F32 b) {
    V3_F32 result;
    result.x = (a.y * b.z) - (a.z * b.y);
    result.y = (a.z * b.x) - (a.x * b.z);
    result.z = (a.x * b.y) - (a.y * b.x);
    return result;
}

inline internal f32 lerp(f32 a, f32 b, f32 t) {
    f32 result = (1.0f - t) * a + b * t;
    return result;
}

inline internal V2_F32 lerp_v2_f32(V2_F32 a, V2_F32 b, f32 t) {
    V2_F32 result;
    result = add_v2_f32(mul_v2_f32_f(a, (1.0f - t)), mul_v2_f32_f(b, t));
    return result;
}

inline internal V3_F32 lerp_v3_f32(V3_F32 a, V3_F32 b, f32 t) {
    V3_F32 result;
    result = add_v3_f32(mul_v3_f32_f(a, (1.0f - t)), mul_v3_f32_f(b, t));
    return result;
}

inline internal V4_F32 lerp_v4_f32(V4_F32 a, V4_F32 b, f32 t) {
    V4_F32 result;
    result = add_v4_f32(mul_v4_f32_f(a, (1.0f - t)), mul_v4_f32_f(b, t));
    return result;
}

inline internal V2_S32 add_v2_s32(V2_S32 a, V2_S32 b) {
    V2_S32 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

inline internal V3_S32 add_v3_s32(V3_S32 a, V3_S32 b) {
    V3_S32 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

inline internal V4_S32 add_v4_s32(V4_S32 a, V4_S32 b) {
    V4_S32 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

inline internal V2_S32 sub_v2_s32(V2_S32 a, V2_S32 b) {
    V2_S32 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

inline internal V3_S32 sub_v3_s32(V3_S32 a, V3_S32 b) {
    V3_S32 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

inline internal V4_S32 sub_v4_s32(V4_S32 a, V4_S32 b) {
    V4_S32 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return result;
}

inline internal V2_S32 mul_v2_s32_s(V2_S32 v, s32 s) {
    V2_S32 result;
    result.x = (int)(v.x * s);
    result.y = (int)(v.y * s);
    return result;
}

inline internal V3_S32 mul_v3_s32_s(V3_S32 v, s32 s) {
    V3_S32 result;
    result.x = (int)(v.x * s);
    result.y = (int)(v.y * s);
    result.z = (int)(v.z * s);
    return result;
}

inline internal V4_S32 mul_v4_s32_s(V4_S32 v, s32 s) {
    V4_S32 result;
    result.x = (int)(v.x * s);
    result.y = (int)(v.y * s);
    result.z = (int)(v.z * s);
    result.w = (int)(v.w * s);
    return result;
}

inline internal V2_F32 add_v2_f32(V2_F32 a, V2_F32 b) {
    V2_F32 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

inline internal V3_F32 add_v3_f32(V3_F32 a, V3_F32 b) {
    V3_F32 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result; 
}

inline internal V4_F32 add_v4_f32(V4_F32 a, V4_F32 b) {
    V4_F32 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

inline internal V2_F64 add_v2_f64(V2_F64 a, V2_F64 b) {
    V2_F64 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

inline internal V3_F64 add_v3_f64(V3_F64 a, V3_F64 b) {
    V3_F64 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result; 
}

inline internal V4_F64 add_v4_f64(V4_F64 a, V4_F64 b) {
    V4_F64 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

inline internal V2_F32 sub_v2_f32(V2_F32 a, V2_F32 b) {
    V2_F32 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

inline internal V3_F32 sub_v3_f32(V3_F32 a, V3_F32 b) {
    V3_F32 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result; 
}

inline internal V4_F32 sub_v4_f32(V4_F32 a, V4_F32 b) {
    V4_F32 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return result;
}

inline internal V2_F64 sub_v2_f64(V2_F64 a, V2_F64 b) {
    V2_F64 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

inline internal V3_F64 sub_v3_f64(V3_F64 a, V3_F64 b) {
    V3_F64 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result; 
}

inline internal V4_F64 sub_v4_f64(V4_F64 a, V4_F64 b) {
    V4_F64 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

inline internal V2_F32 negate_v2_f32(V2_F32 v) {
    V2_F32 result;
    result.x = -v.x;
    result.y = -v.y;
    return result;
}

inline internal V3_F32 negate_v3_f32(V3_F32 v) {
    V3_F32 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}

inline internal V4_F32 negate_v4_f32(V4_F32 v) {
    V4_F32 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    result.w = -v.w;
    return result;
}

inline internal V2_F32 mul_v2_f32_f(V2_F32 v, f32 s) {
    V2_F32 result;
    result.x = v.x * s;
    result.y = v.y * s;
    return result;
}

inline internal V3_F32 mul_v3_f32_f(V3_F32 v, f32 s) {
    V3_F32 result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    return result;
}

inline internal V4_F32 mul_v4_f32_f(V4_F32 v, f32 s) {
    V4_F32 result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    result.w = v.w * s;
    return result;
}

inline internal V2_F64 mul_v2_f64_f(V2_F64 v, f64 s) {
    V2_F64 result;
    result.x = v.x * s;
    result.y = v.y * s;
    return result;
}

inline internal V3_F64 mul_v3_f64_f(V3_F64 v, f64 s) {
    V3_F64 result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    return result;
}

inline internal V4_F64 mul_v4_f64_f(V4_F64 v, f64 s) {
    V4_F64 result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    result.w = v.w * s;
    return result;
}

inline internal V2_F32 div_v2_f32_f(V2_F32 v, f32 s) {
    V2_F32 result;
    result.x = v.x / s;
    result.y = v.y / s;
    return result;
}

inline internal V3_F32 div_v3_f32_f(V3_F32 v, f32 s) {
    V3_F32 result;
    result.x = v.x / s;
    result.y = v.y / s;
    result.z = v.z / s;
    return result;
}

inline internal V4_F32 div_v4_f32_f(V4_F32 v, f32 s) {
    V4_F32 result;
    result.x = v.x / s;
    result.y = v.y / s;
    result.z = v.z / s;
    result.w = v.w / s;
    return result;
}

//@Note Function overloads
#ifdef __cplusplus

internal V2_F32 v2_f32(V2_F64 v) {V2_F32 result = {(f32)v.x, (f32)v.y}; return result;}
internal V3_F32 v3_f32(V3_F64 v) {V3_F32 result = {(f32)v.x, (f32)v.y, (f32)v.z}; return result;}
internal V4_F32 v4_f32(V4_F64 v) {V4_F32 result = {(f32)v.x, (f32)v.y, (f32)v.z, (f32)v.w}; return result;}

inline internal V4_F32 mul(M4_F32 m, V4_F32 v) {V4_F32 result = mul_m4_v4_f32(m, v); return result;}
inline internal M4_F32 mul(M4_F32 a, M4_F32 b) {M4_F32 result = mul_m4_f32(a, b); return result;}

inline internal V2_F32 normalize(V2_F32 v) {V2_F32 result = normalize_v2_f32(v); return result;}
inline internal V3_F32 normalize(V3_F32 v) {V3_F32 result = normalize_v3_f32(v); return result;}
inline internal V4_F32 normalize(V4_F32 v) {V4_F32 result = normalize_v4_f32(v); return result;}

inline internal f32 dot(V2_F32 a, V2_F32 b) {f32 result = dot_v2_f32(a, b); return result;}
inline internal f32 dot(V3_F32 a, V3_F32 b) {f32 result = dot_v3_f32(a, b); return result;}
inline internal f32 dot(V4_F32 a, V4_F32 b) {f32 result = dot_v4_f32(a, b); return result;}

inline internal f32 length(V2_F32 v) {f32 result = length_v2_f32(v); return result;}
inline internal f32 length(V3_F32 v) {f32 result = length_v3_f32(v); return result;}
inline internal f32 length(V4_F32 v) {f32 result = length_v4_f32(v); return result;}

inline internal f32 length2(V2_F32 v) {f32 result = length2_v2_f32(v); return result;}
inline internal f32 length2(V3_F32 v) {f32 result = length2_v3_f32(v); return result;}
inline internal f32 length2(V4_F32 v) {f32 result = length2_v4_f32(v); return result;}

inline internal V2_F32 lerp(V2_F32 a, V2_F32 b, f32 t) {V2_F32 result = lerp_v2_f32(a, b, t); return result;}
inline internal V3_F32 lerp(V3_F32 a, V3_F32 b, f32 t) {V3_F32 result = lerp_v3_f32(a, b, t); return result;}
inline internal V4_F32 lerp(V4_F32 a, V4_F32 b, f32 t) {V4_F32 result = lerp_v4_f32(a, b, t); return result;}

inline internal V3_F32 cross(V3_F32 a, V3_F32 b) {V3_F32 result = cross_v3_f32(a, b); return result;}
#endif // __cplusplus
