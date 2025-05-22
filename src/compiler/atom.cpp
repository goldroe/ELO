global Atom_Table *g_atom_table;
global Arena *g_atom_arena;

internal inline Allocator atom_allocator() {
    Allocator result;
    result.data = (void *)g_atom_arena;
    result.proc = arena_allocator_proc;
    return result;
}

internal bool atoms_match(Atom *a, Atom *b) {
    return a == b;
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
            strncmp(atom->data, (char *)string.data, string.count) == 0) {
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

internal void atom_init() {
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

    atom_keyword(TOKEN_NULL,     str8_lit("null"));
    atom_keyword(TOKEN_ENUM,     str8_lit("enum"));
    atom_keyword(TOKEN_STRUCT,   str8_lit("struct"));
    atom_keyword(TOKEN_TRUE,     str8_lit("true"));
    atom_keyword(TOKEN_FALSE,    str8_lit("false"));
    atom_keyword(TOKEN_IF,       str8_lit("if"));
    atom_keyword(TOKEN_ELSE,     str8_lit("else"));
    atom_keyword(TOKEN_IFCASE,   str8_lit("ifcase"));
    atom_keyword(TOKEN_CASE,     str8_lit("case"));
    atom_keyword(TOKEN_WHILE,    str8_lit("while"));
    atom_keyword(TOKEN_FOR,      str8_lit("for"));
    atom_keyword(TOKEN_BREAK,    str8_lit("break"));
    atom_keyword(TOKEN_CONTINUE, str8_lit("continue"));
    atom_keyword(TOKEN_RETURN,   str8_lit("return"));
    atom_keyword(TOKEN_CAST,     str8_lit("cast"));
    atom_keyword(TOKEN_OPERATOR, str8_lit("operator"));
    atom_keyword(TOKEN_IN,       str8_lit("in"));

    atom_directive(TOKEN_LOAD,   str8_lit("#load"));
    atom_directive(TOKEN_IMPORT, str8_lit("#import"));
    atom_directive(TOKEN_THROUGH, str8_lit("#through"));
    atom_directive(TOKEN_FOREIGN, str8_lit("#foreign"));
}

