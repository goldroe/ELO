#ifndef BASE_CORE_H
#define BASE_CORE_H

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Abs(x) ((x) >= 0 ? (x) : -(x))
#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) >= (b) ? (a) : (b))
#define Clamp(v, min, max) ((v) < (min) ? (min) : (v) > (max) ? (max) : (v))
#define ClampBot(v, bottom) (Max(v, bottom))
#define ClampTop(v, top) (Min(v, top))

#define IsPow2(x)          ((x)!=0 && ((x)&((x)-1))==0)
// #define AlignForward(x, a) ((x)+(a)-((x)&((a)-1)))
#define AlignForward(x,b)     (((x) + (b) - 1)&(~((b) - 1)))
#define AlignDownPow2(x,b) ((x)&(~((b) - 1)))

#ifdef __cplusplus
#define EnumDefineFlagOperators(ENUMTYPE)       \
extern "C++" { \
inline ENUMTYPE operator  | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) | ((int)b)); } \
inline ENUMTYPE &operator |=(ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) |= ((int)b)); } \
inline ENUMTYPE operator  & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) & ((int)b)); } \
inline ENUMTYPE &operator &=(ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) &= ((int)b)); } \
inline ENUMTYPE operator  ~ (ENUMTYPE a) { return ENUMTYPE(~((int)a)); } \
inline ENUMTYPE operator  ^ (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) ^ ((int)b)); } \
inline ENUMTYPE &operator ^=(ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) ^= ((int)b)); } \
}
#else
#define EnumDefineFlagOperators(ENUMTYPE) // NOP, C allows these operators.
#endif 

//@Note Linked List helper macros

#define DLLRemove(f,l,n,next,prev) (((n)==(f) ? (f) = (n)->next : 0),   \
        ((n) == (l) ? (l) = (l)->prev : 0),                             \
        ((n)->prev != NULL ? ((n)->prev->next = (n)->next) : 0),        \
        ((n)->next != NULL ? ((n)->next->prev = (n)->prev) : 0))
#define DLLPushBack(f,l,n,next,prev) (((f)==NULL)?\
        ((f)=(l)=(n),(n)->prev=NULL,(n)->next=NULL):\
        ((l)->next=(n),(n)->prev=(l),(l)=(n),(n)->next=NULL))
#define DLLPushFront(f,l,n,next, prev) (((f)==NULL)?    \
        ((f)=(l)=(n), (n)->prev=NULL,(n)->next=NULL):\
        ((n)->next=(f),(f)->prev=n,(f)=(n),(n)->prev=NULL))

#define SLLQueuePush(f,l,n) (((f)==NULL)?\
        ((f)=(l)=(n),(n)->next=NULL):           \
        ((l)->next=(n),(l)=(n),(n)->next=NULL))
#define SLLQueuePop(f,l,n) (((f)==(l))?         \
        (((f)=NULL, (l)=NULL)):                 \
        ((f)=(f)->next))

#define SLLStackPush(f,n) ((n)->next = (f),(f)=(n))
#define SLLStackPop(f)    ((f) = (f)->next)

#define ArrayCount(array) (sizeof((array)) / sizeof((array)[0]))
#define DeferLoop(begin, end) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))

#define MemoryCopy(dest, src, bytes) (memmove(dest, src, bytes)) 
#define MemoryZero(dest, bytes)      (memset(dest, 0, bytes))

#ifdef _WIN32
#define DebugTrap() __debugbreak()
#elif __linux__
#define DebugTrap() __builtin_trap()
#endif

void AssertMessage(const char *message, const char *file, int line);

#define Assert(cond) if (!(cond)) { \
    AssertMessage(#cond, __FILE__, __LINE__); \
    DebugTrap(); \
} \

#define local_persist static

#define Swap(T,a,b) do{T __t = a; a = b; b = __t;}while(0)
#define quick_sort(Arr,T,N,Func) qsort((Arr), (N), sizeof(T), (Func))

#include <stdint.h>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i8  b8;
typedef i16 b16;
typedef i32 b32;
typedef i64 b64;
typedef float  f32;
typedef double f64;
typedef uintptr_t uintptr;

typedef u64 R_Handle;

enum Axis2 {
    Axis_X,
    Axis_Y,
    Axis_COUNT,
};

union V2_S32 {
    struct {
        i32 x, y;
    };
    int e[2];
    int& operator[](int index) {
        return e[index];
    }
};

union V2_S64 {
    struct {
        i64 x, y;
    };
    i64 e[2];
    i64& operator[](int index) {
        return e[index];
    }
};

union V3_S32 {
    struct {
        i32 x, y, z;
    };
    int e[3];
    int& operator[](int index) {
        return e[index];
    }
};

union V3_S64 {
    struct {
        i64 x, y, z;
    };
    i64 e[3];
    i64& operator[](int index) {
        return e[index];
    }
};

union V4_S32 {
    struct {
        i32 x, y, z, w;
    };
    i32 e[4];
    i32& operator[](int index) {
        return e[index];
    }
};

union V4_S64 {
    struct {
        i64 x, y, z, w;
    };
    i64 e[4];
    i64& operator[](int index) {
        return e[index];
    }
};

union V2_F32 {
    struct {
        f32 x, y;
    };
    f32 e[2];
    f32& operator[](int index) {
        return e[index];
    }
};

union V2_F64 {
    struct {
        f64 x, y;
    };
    f64 e[2];
    f64& operator[](int index) {
        return e[index];
    }
};

union V3_F32 {
    struct {
        f32 x, y, z;
    };
    f32 e[3];
    f32& operator[](int index) {
        return e[index];
    }
};

union V3_F64 {
    struct {
        f64 x, y, z;
    };
    f64 e[3];
    f64& operator[](int index) {
        return e[index];
    }
};

union V4_F32 {
    struct {
        f32 x, y, z, w;
    };
    f32 e[4];
    f32& operator[](int index) {
        return e[index];
    }
};

union V4_F64 {
    struct {
        f64 x, y, z, w;
    };
    f64 e[4];
    f64& operator[](int index) {
        return e[index];
    }
};

union M4_F32 {
    V4_F32 columns[4];
    struct {
        f32 _00, _01, _02, _03;
        f32 _10, _11, _12, _13;
        f32 _20, _21, _22, _23;
        f32 _30, _31, _32, _33;
    };
    f32 e[4][4];
};

struct Txt_Pos {
    u64 col;
    u64 line;
};

union Rng_U64 {
    struct {
        u64 min;
        u64 max;
    };
    u64 v[2];
};

union Rng_S64 {
    struct {
        i64 min;
        i64 max;
    };   
    i64 v[2];
};

union Rect {
    struct {
        f32 x0, y0, x1, y1;
    };
    struct {
        V2_F32 p0;
        V2_F32 p1;
    };
};

enum Face {
    FACE_TOP,
    FACE_BOTTOM,
    FACE_NORTH,
    FACE_SOUTH,
    FACE_EAST,
    FACE_WEST,
    FACE_COUNT
};

union RGBA {
    u32 v;
    struct {
        u8 r, g, b, a;
    };
    u8 e[4];
};


Rng_U64 rng_u64(u64 min, u64 max);
u64 rng_u64_len(Rng_U64 rng);
Rng_S64 rng_s64(i64 min, i64 max);
i64 rng_s64_len(Rng_S64 rng);

#endif // BASE_CORE_H

