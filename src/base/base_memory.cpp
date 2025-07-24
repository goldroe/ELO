#include "base_memory.h"
#include "os/os.h"

Arena *temporary_arena;

void *alloc_align(Allocator allocator, u64 size, int alignment) {
    void *memory = allocator.proc(ALLOCATION_ALLOC, allocator.data, size, alignment, NULL);
    return memory;
}

void *alloc(Allocator allocator, u64 size) {
    return alloc_align(allocator, size, DEFAULT_MEMORY_ALIGNMENT);
}

void free(Allocator allocator, void *memory) {
    allocator.proc(ALLOCATION_FREE, allocator.data, 0, 0, memory);
}

void *allocator_resize(Allocator allocator, void *old_mem, u64 size, int alignment) {
    return allocator.proc(ALLOCATION_RESIZE, allocator.data, size, alignment, old_mem);
}

ALLOCATOR_PROC(arena_allocator_proc) {
    Arena *arena =  (Arena *)data;
    switch (type) {
    case ALLOCATION_ALLOC: {
        void *memory = arena_alloc(arena, size, alignment);
        return memory;
    }
    case ALLOCATION_RESIZE: {
        Assert(0);
        break;
    }
    case ALLOCATION_FREE: {
        Assert(0);
        break;
    }
    case ALLOCATION_FREE_ALL: {
        arena_release(arena);
        break;
    }
    }
    return NULL;
}

ALLOCATOR_PROC(heap_allocator_proc) {
    switch (type) {
    case ALLOCATION_ALLOC: {
        void *memory = malloc(size);
        MemoryZero(memory, size);
        return memory;
    }
    case ALLOCATION_RESIZE: {
        void *memory = realloc(old_mem, size);
        return memory;
    }
    case ALLOCATION_FREE: {
        if (old_mem) free(old_mem);
        break;
    }
    case ALLOCATION_FREE_ALL: {
        if (old_mem) free(old_mem);
        break;
    }
    }
    return NULL;
}


Allocator arena_allocator(Arena *arena) {
    Allocator result;
    result.proc = arena_allocator_proc;
    result.data = (void *)arena;
    return result;
}

Allocator temporary_allocator() {
    Allocator result;
    result.data = (void *)temporary_arena;
    result.proc = arena_allocator_proc;
    return result;
}

Allocator heap_allocator() {
    Allocator result;
    result.proc = heap_allocator_proc;
    result.data = NULL;
    return result;
}

#if defined(OS_WINDOWS)
void *virtual_memory_alloc(u64 size) {
    void *result = NULL;
    result = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (result == NULL) {
        int error = GetLastError();
        fprintf(stderr, "VirtualAlloc failed: %d\n", error);
    }
    return result;
}

void virtual_memory_free(void *memory) {
    if (VirtualFree(memory, 0, MEM_RELEASE) == 0) {
        int error = GetLastError();
        fprintf(stderr, "VirtualFree failed: %d\n", error);
    }
}
#endif

Arena *arena_create(u64 block_size) {
    Arena *arena = alloc_item(heap_allocator(), Arena);
    arena->minimum_block_size = block_size;
    Memory_Block *block = alloc_item(heap_allocator(), Memory_Block);
    block->prev = NULL;
    block->memory = (u8 *)virtual_memory_alloc(block_size);
    block->size = 0;
    block->capacity = block_size;
    arena->curr_block = block;
    return arena;
}

u64 arena_align_forward_offset(Arena *arena, int alignment) {
    u64 offset = 0;
    u64 ptr = (u64)(arena->curr_block->memory + arena->curr_block->size);
    u64 mask = alignment - 1;
    if (ptr & mask) {
        offset = alignment - (ptr & mask);
    }
    return offset;
}

void *arena_alloc(Arena *arena, u64 min_size, int alignment) {
    u64 size = 0;
    if (arena->curr_block) {
        size = min_size + arena_align_forward_offset(arena, alignment);
    }

    if (!arena->curr_block ||
        (arena->curr_block->capacity < arena->curr_block->size + size)) {
        Memory_Block *block = alloc_item(heap_allocator(), Memory_Block);
        if (min_size < arena->minimum_block_size) {
            min_size = arena->minimum_block_size;
        }
        size = min_size + arena_align_forward_offset(arena, alignment);
        block->prev = arena->curr_block;
        block->memory = (u8 *)virtual_memory_alloc(size);
        block->size = 0;
        block->capacity = size;

        arena->curr_block = block;
    }

    Memory_Block *block = arena->curr_block;
    u8 *ptr = block->memory + block->size;
    ptr += arena_align_forward_offset(arena, alignment);
    arena->curr_block->size += size;
    return ptr;
}

void arena_release(Arena *arena) {
    for (Memory_Block *block = arena->curr_block; block; block = block->prev) {
        virtual_memory_free(block->memory);
        free(heap_allocator(), block);
    }
    arena->curr_block = NULL;
}
