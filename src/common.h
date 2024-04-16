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

#endif // COMMON_H