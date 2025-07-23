#if !defined(COMMON_H)
#define COMMON_H

#include "array.h"
#include "base/base_strings.h"

struct Ast;
struct Atom;
struct Decl;
struct Scope;
struct Type;

char *get_scoping_name(Scope *scope);
char *get_name_scoped(Atom *name, Scope *scope);
int get_value_count(Type *type);
int get_total_value_count(Array<Ast*> values);
Type *type_from_index(Type *type, int idx);
Decl *lookup_field(Type *type, Atom *name, bool is_type);

extern bool compiler_dump_IR;
extern Array<String> compiler_link_libraries;




#endif //COMMON_H
