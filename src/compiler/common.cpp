#include "base/base_strings.h"
#include "atom.h"
#include "common.h"
#include "types.h"
#include "decl.h"

char *get_scoping_name(Scope *scope) {
    return (char *)scope->name->data;
    // cstring string = nullptr;
    // while (scope) {
    //     if (!scope->name) {
    //         break;
    //     }

    //     cstring_prepend(&string, scope->name->data); 

    //     if (scope->parent && scope->parent->name) {
    //         cstring_prepend(&string, ".");
    //     }

    //     scope = scope->parent;
    // }
    // return string;
}

char *get_name_scoped(Atom *name, Scope *scope) {
    return (char *)name->data;
//     char *string = make_cstring(name->data);
//     if (scope->name) {
//         cstring_prepend(&string, ".");
//         char *scope_name = get_scoping_name(scope);
//         cstring_prepend(&string, scope_name);
//     }
}

int get_value_count(Type *type) {
    if (type->kind == TYPE_TUPLE) {
        Type_Tuple *tuple = (Type_Tuple *)type;
        return (int)tuple->types.count;
    }
    return 1;
}

int get_total_value_count(Array<Ast*> values) {
    int count = 0;
    for (Ast *v : values) {
        count += get_value_count(v->type);
    }
    return count;
}

Type *type_from_index(Type *type, int idx) {
    if (type->kind == TYPE_TUPLE) {
        Type_Tuple *tuple = (Type_Tuple *)type;
        Assert(idx < tuple->types.count);
        return tuple->types[idx];
    } else {
        Assert(idx == 0);
        return type;
    }
}

Decl *lookup_field(Type *type, Atom *name, bool is_type) {
    Decl *select = nullptr;
    if (is_type) {
        if (is_struct_type(type)) {
            Type_Struct *ts = (Type_Struct *)type;
            Decl *member = scope_find(ts->scope, name);
            if (member && member->kind != DECL_VARIABLE) {
                select = member;
            }
        } else if (is_struct_type(type)) {
            Type_Union *tu = (Type_Union *)type;
            Decl *member = scope_find(tu->scope, name);
            if (member && member->kind != DECL_VARIABLE) {
                select = member;
            }
        } else if (is_enum_type(type)) {
            Type_Enum *et = (Type_Enum *)type;
            select = scope_find(et->scope, name);
        }
    } else if (is_struct_type(type) || is_union_type(type)) {
        Array<Decl*> members;
        if (is_struct_type(type)) {
            members = ((Type_Struct *)type)->members;
        } else if (is_union_type(type)) {
            members = ((Type_Union *)type)->members;
        }

        for (Decl *decl : members) {
            if (is_anonymous(decl)) {
                Decl *member = lookup_field(decl->type, name, is_type);
                if (member && member->kind == DECL_VARIABLE) {
                    select = member;
                }
            } else {
                if (atoms_match(decl->name, name)) {
                    if (decl->kind == DECL_VARIABLE) {
                        select = decl;
                    }
                }
            }
        }
    }
    return select;
}

