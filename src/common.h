#ifndef COMMON_H
#define COMMON_H

#include "types.h"

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))

struct Arena {
    uint8 *cursor;
    uint8 *end;
    uint8 **chunks;
    int chunk_count;

    size_t alignment;
    size_t chunk_size;
};

#define _DEFAULT_ALIGNMENT 8
#define _DEFAULT_CHUNK_SIZE (1024 * 1024)
Arena make_arena(size_t alignment = _DEFAULT_ALIGNMENT, size_t chunk_size = _DEFAULT_CHUNK_SIZE);
void *arena_alloc(Arena *arena, size_t size);

#ifdef __cplusplus
#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) \
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
#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) // NOP, C allows these operators.
#endif 

#endif // COMMON_H
