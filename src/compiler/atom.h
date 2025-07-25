#ifndef ATOM_H
#define ATOM_H

#include "token.h"
#include "lexer.h"

enum Atom_Flags {
    ATOM_FLAG_NIL       = 0,
    ATOM_FLAG_IDENT     = (1<<0),
    ATOM_FLAG_KEYWORD   = (1<<1),
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

extern Atom_Table *g_atom_table;
extern Arena *g_atom_arena;

inline bool atoms_match(Atom *a, Atom *b) {return a == b;}

Atom *atom_create(String string);
void atom_init();

#endif // ATOM_H
