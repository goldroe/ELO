#include "atom.h"

Atom_Table *g_atom_table;
Arena *g_atom_arena;

inline Allocator atom_allocator() {
    Allocator result;
    result.data = (void *)g_atom_arena;
    result.proc = arena_allocator_proc;
    return result;
}

u64 atom_hash(String string) {
    u64 result = 5381;
    for (u64 i = 0; i < string.count; i++) {
        result = ((result << 5) + result) + string.data[i];
    }
    return result;
}

Atom *atom_lookup(String string) {
    u64 hash = atom_hash(string);
    int hash_index = hash % g_atom_table->bucket_count;
    Atom_Bucket *bucket = &g_atom_table->buckets[hash_index];

    Atom *result = NULL;
    for (Atom *atom = bucket->first; atom; atom = atom->next) {
        if ((atom->count == string.count) &&
            strncmp(atom->data, (char *)string.data, string.count) == 0) {
            result = atom;
            break;
        }
    }
    return result;
}

Atom *atom_create(String string) {
    u64 hash = atom_hash(string);
    int hash_index = hash % g_atom_table->bucket_count;
    Atom_Bucket *bucket = &g_atom_table->buckets[hash_index];

    for (Atom *atom = bucket->first; atom; atom = atom->next) {
        if ((atom->count == string.count) &&
            strncmp(atom->data, (char *)string.data, string.count) == 0) {
            return atom;
        }
    }

    Atom *atom = alloc_item(atom_allocator(), Atom);
    atom->data = array_alloc(atom_allocator(), char, string.count + 1);
    atom->flags = ATOM_FLAG_IDENT;
    atom->count = string.count;
    MemoryCopy(atom->data, string.data, string.count);
    atom->data[string.count] = 0;
    SLLQueuePush(bucket->first, bucket->last, atom);
    bucket->count++;
    return atom;
}

Atom *atom_keyword(Token_Kind token, String string) {
    Atom *atom = atom_create(string);
    atom->flags = ATOM_FLAG_KEYWORD;
    atom->token = token;
    return atom;
}

void atom_init() {
    g_atom_arena = arena_create();

    g_atom_table = alloc_item(atom_allocator(), Atom_Table);
    MemoryZero(g_atom_table, sizeof(Atom_Table));
    Atom_Table *table = g_atom_table;
    table->bucket_count = 128;
    table->buckets = array_alloc(atom_allocator(), Atom_Bucket, table->bucket_count);
    for (int i = 0; i < table->bucket_count; i++) {
        Atom_Bucket *bucket = &table->buckets[i];
        bucket->first = bucket->last = NULL;
        bucket->count = 0;
    }

    atom_keyword(TOKEN_ENUM,        str_lit("enum"));
    atom_keyword(TOKEN_STRUCT,      str_lit("struct"));
    atom_keyword(TOKEN_UNION,       str_lit("union"));
    atom_keyword(TOKEN_IF,          str_lit("if"));
    atom_keyword(TOKEN_ELSE,        str_lit("else"));
    atom_keyword(TOKEN_IFCASE,      str_lit("ifcase"));
    atom_keyword(TOKEN_CASE,        str_lit("case"));
    atom_keyword(TOKEN_DO,          str_lit("do"));
    atom_keyword(TOKEN_WHILE,       str_lit("while"));
    atom_keyword(TOKEN_FOR,         str_lit("for"));
    atom_keyword(TOKEN_BREAK,       str_lit("break"));
    atom_keyword(TOKEN_CONTINUE,    str_lit("continue"));
    atom_keyword(TOKEN_FALLTHROUGH, str_lit("fallthrough"));
    atom_keyword(TOKEN_RETURN,      str_lit("return"));
    atom_keyword(TOKEN_DEFER,       str_lit("defer"));
    atom_keyword(TOKEN_CAST,        str_lit("cast"));
    atom_keyword(TOKEN_OPERATOR,    str_lit("operator"));
    atom_keyword(TOKEN_IN,          str_lit("in"));
    atom_keyword(TOKEN_SIZEOF,      str_lit("size_of"));
    atom_keyword(TOKEN_TYPEOF,      str_lit("type_of"));
}
