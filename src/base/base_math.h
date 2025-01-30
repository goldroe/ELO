#ifndef BASE_MATH_H
#define BASE_MATH_H

#include <math.h>
#ifndef PI
    #define PI     3.14159265358979323846f
#endif
#ifndef PI2
    #define PI2    6.28318530717958647693f
#endif
#ifndef PIDIV2
    #define PIDIV2 1.5707963267948966192f
#endif
#ifndef EPSILON
    #define EPSILON 0.000001f
#endif

#define Rect_Zero make_rect(0.0f, 0.0f, 0.0f, 0.0f)
#define Rect_One make_rect(1.0f, 1.0f, 1.0f, 1.0f)

#define floor_f32(x)  floorf(x)
#define trunc_f32(x)  truncf(x)
#define round_f32(x)  roundf(x)
#define mod_f32(x,y)  fmodf(x,y)
#define ceil_f32(x)   ceilf(x)
#define sqrt_f32(x)   sqrtf(x)

#define floor_f64(x)  floor(x)
#define trunc_f64(x)  trunc(x)
#define round_f64(x)  round(x)
#define mod_f64(x,y)  fmod(x,y)
#define ceil_f64(x)   ceil(x)
#define sqrt_f64(x)   sqrt(x)

#define DegToRad(deg) ((deg)*PI/180.f)
#define RadToDeg(rad) ((rad)*180.f/PI)

#define V2_Zero   {0,0}
#define V2_One    {1,1}
#define V2_Left   {-1,0}
#define V2_Right  {1,0}
#define V2_Up     {0,1}
#define V2_Down   {0,-1}
#define V2_Half   {0.5f,0.5f}

#define V3_Zero    {0,0,0}
#define V3_One     {1,1,1}
#define V3_Left    {-1,0,0}
#define V3_Right   {1,0,0}
#define V3_Up      {0,1,0}
#define V3_Down    {0,-1,0}
#define V3_Forward {0,0,1}
#define V3_Back    {0,0,-1}
#define V3_Half    {0.5f,0.5f,0.5f}

#define V4_Zero    {0,0,0,0}
#define V4_One     {1,1,1,1}
#define V4_Left    {-1,0,0,0}
#define V4_Right   {1,0,0,0}
#define V4_Up      {0,1,0,0}
#define V4_Down    {0,-1,0,0}
#define V4_Forward {0,0,1,0}
#define V4_Back    {0,0,-1,0}
#define V4_Half    {0.5f,0.5f,0.5f,0.5f}

inline internal M4_F32 identity_m4_f32();
inline internal M4_F32 mul_m4_f32(M4_F32 left, M4_F32 right);
inline internal V4_F32 mul_m4_v4_f32(M4_F32 m, V4_F32 v);
inline internal M4_F32 transpose_m4_f32(M4_F32 m);
inline internal M4_F32 translate_m4_f32(f32 x, f32 y, f32 z);
inline internal M4_F32 inv_translate_m4_f32(f32 x, f32 y, f32 z);
inline internal M4_F32 scale_m4_f32(f32 x, f32 y, f32 z);
inline internal M4_F32 rotate_rh_zo(f32 angle, V3_F32 axis);
inline internal M4_F32 ortho_rh_zo(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);
inline internal M4_F32 perspective_rh_zo(f32 fov, f32 aspect_ratio, f32 near, f32 far);
inline internal M4_F32 look_at_rh_zo(V3_F32 eye, V3_F32 target, V3_F32 up);
inline internal M4_F32 look_at_lh_zo(V3_F32 eye, V3_F32 target, V3_F32 up);

inline internal V2_F32 add_v2_f32(V2_F32 a, V2_F32 b);
inline internal V3_F32 add_v3_f32(V3_F32 a, V3_F32 b);
inline internal V4_F32 add_v4_f32(V4_F32 a, V4_F32 b);
inline internal V2_F64 add_v2_f64(V2_F64 a, V2_F64 b);
inline internal V3_F64 add_v3_f64(V3_F64 a, V3_F64 b);
inline internal V4_F64 add_v4_f64(V4_F64 a, V4_F64 b);

inline internal V2_F32 sub_v2_f32(V2_F32 a, V2_F32 b);
inline internal V3_F32 sub_v3_f32(V3_F32 a, V3_F32 b);
inline internal V4_F32 sub_v4_f32(V4_F32 a, V4_F32 b);
inline internal V2_F64 sub_v2_f64(V2_F64 a, V2_F64 b);
inline internal V3_F64 sub_v3_f64(V3_F64 a, V3_F64 b);
inline internal V4_F64 sub_v4_f64(V4_F64 a, V4_F64 b);

inline internal V2_F32 negate_v2_f32(V2_F32 v);
inline internal V3_F32 negate_v3_f32(V3_F32 v);
inline internal V4_F32 negate_v4_f32(V4_F32 v);

inline internal V2_F32 mul_v2_f32_f(V2_F32 v, f32 s);
inline internal V3_F32 mul_v3_f32_f(V3_F32 v, f32 s);
inline internal V4_F32 mul_v4_f32_f(V4_F32 v, f32 s);
inline internal V2_F64 mul_v2_f64_f(V2_F64 v, f64 s);
inline internal V3_F64 mul_v3_f64_f(V3_F64 v, f64 s);
inline internal V4_F64 mul_v4_f64_f(V4_F64 v, f64 s);

inline internal V2_F32 div_v2_f32_f(V2_F32 v, f32 s);
inline internal V3_F32 div_v3_f32_f(V3_F32 v, f32 s);
inline internal V4_F32 div_v4_f32_f(V4_F32 v, f32 s);
inline internal V2_F64 div_v2_f64_f(V2_F64 v, f64 s);
inline internal V3_F64 div_v3_f64_f(V3_F64 v, f64 s);
inline internal V4_F64 div_v4_f64_f(V4_F64 v, f64 s);

inline internal f32 length_v2_f32(V2_F32 v);
inline internal f32 length_v3_f32(V3_F32 v);
inline internal f32 length_v4_f32(V4_F32 v);
inline internal f32 length2_v2_f32(V2_F32 v);
inline internal f32 length2_v3_f32(V3_F32 v);
inline internal f32 length2_v4_f32(V4_F32 v);
inline internal V2_F32 normalize_v2_f32(V2_F32 v);
inline internal V3_F32 normalize_v3_f32(V3_F32 v);
inline internal V4_F32 normalize_v4_f32(V4_F32 v);

inline internal f32 lerp(f32 a, f32 b, f32 t);
inline internal V2_F32 lerp_v2_f32(V2_F32 a, V2_F32 b, f32 t);
inline internal V3_F32 lerp_v3_f32(V3_F32 a, V3_F32 b, f32 t);
inline internal V4_F32 lerp_v4_f32(V4_F32 a, V4_F32 b, f32 t);

inline internal f32 dot_v2_f32(V2_F32 a, V2_F32 b);
inline internal f32 dot_v3_f32(V3_F32 a, V3_F32 b);
inline internal f32 dot_v4_f32(V4_F32 a, V4_F32 b);

inline internal V3_F32 cross_v3_f32(V3_F32 a, V3_F32 b);

//@Note Function overloads
#ifdef __cplusplus
inline internal V2_F32 normalize(V2_F32 v);
inline internal V3_F32 normalize(V3_F32 v);
inline internal V4_F32 normalize(V4_F32 v);
inline internal f32 dot(V2_F32 a, V2_F32 b);
inline internal f32 dot(V3_F32 a, V3_F32 b);
inline internal f32 dot(V4_F32 a, V4_F32 b);
inline internal V2_F32 lerp(V2_F32 a, V2_F32 b, f32 t);
inline internal V3_F32 lerp(V3_F32 a, V3_F32 b, f32 t);
inline internal V4_F32 lerp(V4_F32 a, V4_F32 b, f32 t);
inline internal f32 length(V2_F32 v);
inline internal f32 length(V3_F32 v);
inline internal f32 length(V4_F32 v);
inline internal f32 length2(V2_F32 v);
inline internal f32 length2(V3_F32 v);
inline internal f32 length2(V4_F32 v);

inline internal V2_S32 add_v2_s32(V2_S32 a, V2_S32 b);
inline internal V3_S32 add_v3_s32(V3_S32 a, V3_S32 b);
inline internal V4_S32 add_v4_s32(V4_S32 a, V4_S32 b);

inline internal V2_S32 sub_v2_s32(V2_S32 a, V2_S32 b);
inline internal V3_S32 sub_v3_s32(V3_S32 a, V3_S32 b);
inline internal V4_S32 sub_v4_s32(V4_S32 a, V4_S32 b);

inline internal V2_S32 mul_v2_s32_s(V2_S32 v, s32 s);
inline internal V3_S32 mul_v3_s32_s(V3_S32 v, s32 s);
inline internal V4_S32 mul_v4_s32_s(V4_S32 v, s32 s);

//@Note Operator overloads
inline V2_S32 operator+(V2_S32 a, V2_S32 b) {V2_S32 result = add_v2_s32(a, b); return result;}
inline V3_S32 operator+(V3_S32 a, V3_S32 b) {V3_S32 result = add_v3_s32(a, b); return result;}
inline V4_S32 operator+(V4_S32 a, V4_S32 b) {V4_S32 result = add_v4_s32(a, b); return result;}

inline V2_S32 operator-(V2_S32 a, V2_S32 b) {V2_S32 result = sub_v2_s32(a, b); return result;}
inline V3_S32 operator-(V3_S32 a, V3_S32 b) {V3_S32 result = sub_v3_s32(a, b); return result;}
inline V4_S32 operator-(V4_S32 a, V4_S32 b) {V4_S32 result = sub_v4_s32(a, b); return result;}

inline V2_S32 operator+=(V2_S32 &a, V2_S32 b) {a = a + b; return a;}
inline V3_S32 operator+=(V3_S32 &a, V3_S32 b) {a = a + b; return a;}
inline V4_S32 operator+=(V4_S32 &a, V4_S32 b) {a = a + b; return a;}

inline V2_S32 operator-=(V2_S32 &a, V2_S32 b) {a = a - b; return a;}
inline V3_S32 operator-=(V3_S32 &a, V3_S32 b) {a = a - b; return a;}
inline V4_S32 operator-=(V4_S32 &a, V4_S32 b) {a = a - b; return a;}

inline V2_S32 operator*(V2_S32 v, s32 f) {V2_S32 result = mul_v2_s32_s(v, f); return result;}
inline V3_S32 operator*(V3_S32 v, s32 f) {V3_S32 result = mul_v3_s32_s(v, f); return result;}
inline V4_S32 operator*(V4_S32 v, s32 f) {V4_S32 result = mul_v4_s32_s(v, f); return result;}

inline V2_F32 operator-(V2_F32 v) {V2_F32 result = negate_v2_f32(v); return result;}
inline V3_F32 operator-(V3_F32 v) {V3_F32 result = negate_v3_f32(v); return result;}
inline V4_F32 operator-(V4_F32 v) {V4_F32 result = negate_v4_f32(v); return result;}

inline V2_F32 operator+(V2_F32 a, V2_F32 b) {V2_F32 result = add_v2_f32(a, b); return result;}
inline V3_F32 operator+(V3_F32 a, V3_F32 b) {V3_F32 result = add_v3_f32(a, b); return result;}
inline V4_F32 operator+(V4_F32 a, V4_F32 b) {V4_F32 result = add_v4_f32(a, b); return result;}
inline V2_F64 operator+(V2_F64 a, V2_F64 b) {V2_F64 result = add_v2_f64(a, b); return result;}
inline V3_F64 operator+(V3_F64 a, V3_F64 b) {V3_F64 result = add_v3_f64(a, b); return result;}
inline V4_F64 operator+(V4_F64 a, V4_F64 b) {V4_F64 result = add_v4_f64(a, b); return result;}

inline V2_F32 operator-(V2_F32 a, V2_F32 b) {V2_F32 result = sub_v2_f32(a, b); return result;}
inline V3_F32 operator-(V3_F32 a, V3_F32 b) {V3_F32 result = sub_v3_f32(a, b); return result;}
inline V4_F32 operator-(V4_F32 a, V4_F32 b) {V4_F32 result = sub_v4_f32(a, b); return result;}
inline V2_F64 operator-(V2_F64 a, V2_F64 b) {V2_F64 result = sub_v2_f64(a, b); return result;}
inline V3_F64 operator-(V3_F64 a, V3_F64 b) {V3_F64 result = sub_v3_f64(a, b); return result;}
inline V4_F64 operator-(V4_F64 a, V4_F64 b) {V4_F64 result = sub_v4_f64(a, b); return result;}

inline V2_F32 operator+=(V2_F32 &a, V2_F32 b) {a = a + b; return a;}
inline V3_F32 operator+=(V3_F32 &a, V3_F32 b) {a = a + b; return a;}
inline V4_F32 operator+=(V4_F32 &a, V4_F32 b) {a = a + b; return a;}
inline V2_F64 operator+=(V2_F64 &a, V2_F64 b) {a = a + b; return a;}
inline V3_F64 operator+=(V3_F64 &a, V3_F64 b) {a = a + b; return a;}
inline V4_F64 operator+=(V4_F64 &a, V4_F64 b) {a = a + b; return a;}

inline V2_F32 operator-=(V2_F32 &a, V2_F32 b) {a = a - b; return a;}
inline V3_F32 operator-=(V3_F32 &a, V3_F32 b) {a = a - b; return a;}
inline V4_F32 operator-=(V4_F32 &a, V4_F32 b) {a = a - b; return a;}
inline V2_F64 operator-=(V2_F64 &a, V2_F64 b) {a = a - b; return a;}
inline V3_F64 operator-=(V3_F64 &a, V3_F64 b) {a = a - b; return a;}
inline V4_F64 operator-=(V4_F64 &a, V4_F64 b) {a = a - b; return a;}

inline V2_F32 operator*(V2_F32 v, f32 s) {V2_F32 result = mul_v2_f32_f(v, s); return result;}
inline V3_F32 operator*(V3_F32 v, f32 s) {V3_F32 result = mul_v3_f32_f(v, s); return result;}
inline V4_F32 operator*(V4_F32 v, f32 s) {V4_F32 result = mul_v4_f32_f(v, s); return result;}
inline V2_F64 operator*(V2_F64 v, f64 s) {V2_F64 result = mul_v2_f64_f(v, s); return result;}
inline V3_F64 operator*(V3_F64 v, f64 s) {V3_F64 result = mul_v3_f64_f(v, s); return result;}
inline V4_F64 operator*(V4_F64 v, f64 s) {V4_F64 result = mul_v4_f64_f(v, s); return result;}

inline V2_F32 operator*(f32 s, V2_F32 v) {V2_F32 result = mul_v2_f32_f(v, s); return result;}
inline V3_F32 operator*(f32 s, V3_F32 v) {V3_F32 result = mul_v3_f32_f(v, s); return result;}
inline V4_F32 operator*(f32 s, V4_F32 v) {V4_F32 result = mul_v4_f32_f(v, s); return result;}
inline V2_F64 operator*(f64 s, V2_F64 v) {V2_F64 result = mul_v2_f64_f(v, s); return result;}
inline V3_F64 operator*(f64 s, V3_F64 v) {V3_F64 result = mul_v3_f64_f(v, s); return result;}
inline V4_F64 operator*(f64 s, V4_F64 v) {V4_F64 result = mul_v4_f64_f(v, s); return result;}

inline V2_F32 operator/(V2_F32 v, f32 s) {V2_F32 result = div_v2_f32_f(v, s); return result;}
inline V3_F32 operator/(V3_F32 v, f32 s) {V3_F32 result = div_v3_f32_f(v, s); return result;}
inline V4_F32 operator/(V4_F32 v, f32 s) {V4_F32 result = div_v4_f32_f(v, s); return result;}

inline V2_F32 operator*=(V2_F32 &v, f32 s) {v = v * s; return v;}
inline V3_F32 operator*=(V3_F32 &v, f32 s) {v = v * s; return v;}
inline V4_F32 operator*=(V4_F32 &v, f32 s) {v = v * s; return v;}

inline V2_F32 operator/=(V2_F32 &v, f32 s) {v = v / s; return v;}
inline V3_F32 operator/=(V3_F32 &v, f32 s) {v = v / s; return v;}
inline V4_F32 operator/=(V4_F32 &v, f32 s) {v = v / s; return v;}

inline bool operator==(V2_F32 a, V2_F32 b) {bool result = a.x == b.x && a.y == b.y; return result;}
inline bool operator==(V3_F32 a, V3_F32 b) {bool result = a.x == b.x && a.y == b.y && a.z == b.z; return result;}
inline bool operator==(V4_F32 a, V4_F32 b) {bool result = a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; return result;}

inline bool operator!=(V2_F32 a, V2_F32 b) {return !(a == b);}
inline bool operator!=(V3_F32 a, V3_F32 b) {return !(a == b);}
inline bool operator!=(V4_F32 a, V4_F32 b) {return !(a == b);}

inline M4_F32 operator*(M4_F32 a, M4_F32 b) {M4_F32 result = mul_m4_f32(a, b); return result;}
#endif // __cplusplus

#endif // BASE_MATH_H
