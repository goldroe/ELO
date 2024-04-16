#include <assert.h>
#include <stdlib.h>
#include "common.h"

Arena make_arena(size_t alignment, size_t chunk_size) {
    Arena arena{};
    arena.alignment = alignment;
    arena.chunk_size = chunk_size;
    return arena;
}

uintptr_t align_forward(uintptr_t cursor, size_t alignment) {
    assert(alignment % 2 == 0);
    uintptr_t modulo;
    // modulo = cursor % alignment;
    modulo = cursor & (alignment - 1);
    cursor += alignment - modulo;
    return cursor;
}

void arena_grow(Arena *arena, size_t min_size) {
    size_t size = align_forward((uintptr_t)MAX(min_size, arena->chunk_size), arena->alignment);
    arena->cursor = (uint8 *)calloc(size, 1);
    arena->end = arena->cursor + size;
    arena->chunk_count++;
    arena->chunks = (uint8 **)realloc(arena->chunks, arena->chunk_count * sizeof(uint8 *));
}

void *arena_alloc(Arena *arena, size_t size) {
    if (size > (size_t)(arena->end - arena->cursor)) {
        arena_grow(arena, size);
    }
    uint8 *ptr = arena->cursor;
    arena->cursor = (uint8 *)align_forward((uintptr_t)arena->cursor + size, arena->alignment);
    return ptr;
}
