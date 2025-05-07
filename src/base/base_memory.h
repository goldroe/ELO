#if !defined(BASE_MEMORY_H)
#define BASE_MEMORY_H

#define is_pow2(x)          ((x)!=0 && ((x)&((x)-1))==0)
#define AlignDownPow2(x,b) ((x)&(~((b) - 1)))
// #define AlignForward(x, a) ((x)+(a)-((x)&((a)-1)))
#define align_forward(x,a) (((x) + (a) - 1)&(~((a) - 1)))

#define MINIMUM_ARENA_BLOCK_SIZE 8ll * 1024ll * 1024ll
#define DEFAULT_MEMORY_ALIGNMENT (2 * sizeof(void *))

enum Allocation_Type {
    ALLOCATION_ALLOC,
    ALLOCATION_RESIZE,
    ALLOCATION_FREE,
    ALLOCATION_FREE_ALL,
};

#define ALLOCATOR_PROC(Name) void *Name(Allocation_Type type, void *data, u64 size, int alignment, void *old_mem) 
typedef ALLOCATOR_PROC(Allocator_Proc);

struct Allocator {
    Allocator_Proc *proc;
    void *data;
};

internal void *alloc(Allocator allocator, u64 size);
internal void free(Allocator allocator, void *memory);

#define array_alloc(Alloc, T, Count) (T*)alloc(Alloc, sizeof(T) * (Count))
#define alloc_item(Alloc, T) (T*)alloc(Alloc, sizeof(T))

struct Memory_Block {
    Memory_Block *prev;
    u8 *memory;
    u64 size;
    u64 capacity;
};

struct Arena {
    Memory_Block *curr_block;
    u64 minimum_block_size;
};

internal inline Allocator heap_allocator();
internal inline Allocator temporary_allocator();

internal Arena *arena_create(u64 block_size = MINIMUM_ARENA_BLOCK_SIZE);
internal void *arena_alloc(Arena *arena, u64 min_size, int alignment);
internal void arena_release(Arena *arena);

#endif // BASE_MEMORY_H
