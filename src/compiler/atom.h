#ifndef ATOM_H
#define ATOM_H

enum Atom_Flags {
    ATOM_FLAG_NIL       = 0,
    ATOM_FLAG_IDENT     = (1<<0),
    ATOM_FLAG_KEYWORD   = (1<<1),
    ATOM_FLAG_DIRECTIVE = (1<<2),
};
EnumDefineFlagOperators(Atom_Flags);

struct Atom {
    Atom *next;
    char *data;
    u64 count;
    Atom_Flags flags;
    Token_Kind token;
};

struct Atom_Bucket {
    Atom *first;
    Atom *last;
    int count;
};

struct Atom_Table {
    Atom_Bucket *buckets;
    int bucket_count;
};

internal Atom *atom_create(String8 string);

#endif // ATOM_H
