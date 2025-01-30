global Atom_Table *g_atom_table;
global Arena *g_atom_arena;

internal bool atoms_match(Atom *a, Atom *b) {
    return a == b;
}

internal void atom_init() {
    g_atom_arena = arena_alloc(get_virtual_allocator(), MB(1));

    g_atom_table = push_array(g_atom_arena, Atom_Table, 1);
    Atom_Table *table = g_atom_table;
    table->bucket_count = 128;
    table->buckets = push_array(g_atom_arena, Atom_Bucket, table->bucket_count);
}

internal u64 atom_hash(String8 string) {
    u64 result = 5381;
    for (u64 i = 0; i < string.count; i++) {
        result = ((result << 5) + result) + string.data[i];
    }
    return result;
}

internal Atom *atom_lookup(String8 string) {
    u64 hash = atom_hash(string);
    int hash_index = hash % g_atom_table->bucket_count;
    Atom_Bucket *bucket = &g_atom_table->buckets[hash_index];

    Atom *result = NULL;
    for (Atom *atom = bucket->first; atom; atom = atom->next) {
        if ((atom->count == string.count) &&
            strncmp((char *)atom->data, (char *)string.data, string.count) == 0) {
            result = atom;
            break;
        }
    }
    return result;
}

internal Atom *atom_create(String8 string) {
    u64 hash = atom_hash(string);
    int hash_index = hash % g_atom_table->bucket_count;
    Atom_Bucket *bucket = &g_atom_table->buckets[hash_index];

    for (Atom *atom = bucket->first; atom; atom = atom->next) {
        if ((atom->count == string.count) &&
            strncmp((char *)atom->data, (char *)string.data, string.count) == 0) {
            return atom;
        }
    }

    u64 mem_size = sizeof(Atom) + string.count + 1;
    Atom *atom = (Atom *)arena_push(g_atom_arena, mem_size);
    atom->flags = ATOM_FLAG_IDENT;
    atom->count = string.count;
    MemoryCopy(atom->data, string.data, string.count);
    atom->data[string.count] = 0;
    SLLQueuePush(bucket->first, bucket->last, atom);
    bucket->count++;
    return atom;
}

internal Atom *atom_keyword(Token_Kind token, String8 string) {
    Atom *atom = atom_create(string);
    atom->flags = ATOM_FLAG_KEYWORD;
    atom->token = token;
    return atom;
}

internal Atom *atom_directive(Token_Kind token, String8 string) {
    Atom *atom = atom_create(string);
    atom->flags = ATOM_FLAG_DIRECTIVE;
    atom->token = token;
    return atom;
}
