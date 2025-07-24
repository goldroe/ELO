#include "base/base_strings.h"

#include "atom.h"
#include "ast.h"
#include "types.h"

Arena *g_ast_arena;
u64 g_ast_counter;

const char *ast_strings[] = {
#define AST_KIND(K,S) S
    AST_KINDS
#undef AST_KIND
};

bool is_ast_type(Ast *node) {
    switch (node->kind) {
    case AST_STRUCT_TYPE:
    case AST_ENUM_TYPE:
    case AST_UNION_TYPE:
    case AST_ARRAY_TYPE:
        return true;
    }
    return false;
}

bool is_ast_stmt(Ast *node) {
    return AST_STMT_BEGIN < node->kind && node->kind < AST_STMT_END;
}

inline Allocator ast_allocator() {
    return arena_allocator(g_ast_arena);
}

inline Ast *ast__init(Ast *node, Source_File *f) {
    node->file = f;
    node->id = g_ast_counter++;
    return node;
}

inline Ast *ast_alloc(u64 size, int alignment) {
    Ast *node = (Ast *)arena_alloc(g_ast_arena, size, alignment);
    return node;
}

Ast_Root *ast_root(Source_File *f, Array<Ast*> decls) {
    Ast_Root *node = AST_NEW(f, Ast_Root);
    node->decls = decls;
    return node;
}

Ast_Empty_Stmt *ast_empty_stmt(Source_File *f, Token token) {
    Ast_Empty_Stmt *node = AST_NEW(f, Ast_Empty_Stmt);
    node->token = token;
    return node;
}

Ast_Bad_Expr *ast_bad_expr(Source_File *f, Token start, Token end) {
    Ast_Bad_Expr *node = AST_NEW(f, Ast_Bad_Expr);
    node->poison();
    node->start = start;
    node->end = end;
    return node;
}

Ast_Bad_Stmt *ast_bad_stmt(Source_File *f, Token start, Token end) {
    Ast_Bad_Stmt *node = AST_NEW(f, Ast_Bad_Stmt);
    node->poison();
    node->start = start;
    node->end = end;
    return node;
}

Ast_Bad_Decl *ast_bad_decl(Source_File *f, Token start, Token end) {
    Ast_Bad_Decl *node = AST_NEW(f, Ast_Bad_Decl);
    node->poison();
    node->start = start;
    node->end = end;
    return node;
}

Ast_Return *ast_return_stmt(Source_File *f, Token token, Array<Ast*> values) {
    Ast_Return *node = AST_NEW(f, Ast_Return);
    node->values = values;
    node->token = token;
    return node;
}

Ast_Continue *ast_continue_stmt(Source_File *f, Token token) {
    Ast_Continue *node = AST_NEW(f, Ast_Continue);
    node->token = token;
    return node;
}

Ast_Break *ast_break_stmt(Source_File *f, Token token) {
    Ast_Break *node = AST_NEW(f, Ast_Break);
    node->token = token;
    return node;
}

Ast_Fallthrough *ast_fallthrough_stmt(Source_File *f, Token token) {
    Ast_Fallthrough *node = AST_NEW(f, Ast_Fallthrough);
    node->token = token;
    return node;
}

Ast_Defer *ast_defer_stmt(Source_File *f, Token token, Ast *stmt) {
    Ast_Defer *node = AST_NEW(f, Ast_Defer);
    node->token = token;
    node->stmt = stmt;
    return node;
}

Ast_Proc_Lit *ast_proc_lit(Source_File *f, Ast_Proc_Type *typespec, Ast_Block *body) {
    Ast_Proc_Lit *node = AST_NEW(f, Ast_Proc_Lit);
    node->typespec = typespec;
    node->body = body;
    array_init(&node->local_vars, heap_allocator());
    return node;
}

Ast_Enum_Field *ast_enum_field(Source_File *f, Ast_Ident *ident, Ast *expr) {
    Ast_Enum_Field *node = AST_NEW(f, Ast_Enum_Field);
    node->mode = ADDRESSING_CONSTANT;
    node->name = ident;
    node->expr = expr;
    return node;
}

Ast_Param *ast_param(Source_File *f, Ast_Ident *name, Ast *typespec, bool is_variadic) {
    Ast_Param *node = AST_NEW(f, Ast_Param);
    node->name = name;
    node->typespec = typespec;
    node->is_variadic = is_variadic;
    return node;
}

Ast_Ident *ast_ident(Source_File *f, Token token) {
    Ast_Ident *node = AST_NEW(f, Ast_Ident);
    node->name = token.name;
    node->token = token;
    return node;
}

Ast_Literal *ast_literal(Source_File *f, Token token) {
    Ast_Literal *node = AST_NEW(f, Ast_Literal);
    node->value = token.value;
    node->token = token;
    return node;
}

Ast_Compound_Literal *ast_compound_literal(Source_File *f, Token token, Token open, Token close, Ast *typespec, Array<Ast*> elements) {
    Ast_Compound_Literal *node = AST_NEW(f, Ast_Compound_Literal);
    node->elements = elements;
    node->typespec = typespec;
    node->token = token;
    node->open = open;
    node->close = close;
    return node;
}

Ast_Uninit *ast_uninit_expr(Source_File *f, Token token) {
    Ast_Uninit *node = AST_NEW(f, Ast_Uninit);
    node->token = token;
    return node;
}

Ast_Paren *ast_paren_expr(Source_File *f, Token open, Token close, Ast *elem) {
    Ast_Paren *node = AST_NEW(f, Ast_Paren);
    node->elem = elem;
    node->open = open;
    node->close = close;
    return node;
}

Ast_Unary *ast_unary_expr(Source_File *f, Token token, OP op, Ast *elem) {
    Ast_Unary *node = AST_NEW(f, Ast_Unary);
    node->op = op;
    node->elem = elem;
    node->token = token;
    return node;
}

Ast_Star_Expr *ast_star_expr(Source_File *f, Token token, Ast *elem) {
    Ast_Star_Expr *node = AST_NEW(f, Ast_Star_Expr);
    node->elem = elem;
    node->token = token;
    return node;
}

Ast_Range *ast_range_expr(Source_File *f, Token token, Ast *lhs, Ast *rhs) {
    Ast_Range *node = AST_NEW(f, Ast_Range);
    node->lhs = lhs;
    node->rhs = rhs;
    node->token = token;
    return node;
}

Ast_Deref *ast_deref_expr(Source_File *f, Token token, Ast *elem) {
    Ast_Deref *node = AST_NEW(f, Ast_Deref);
    node->elem = elem;
    node->token = token;
    return node;
}

Ast_Cast *ast_cast_expr(Source_File *f, Token token, Ast *typespec, Ast *elem) {
    Ast_Cast *node = AST_NEW(f, Ast_Cast);
    node->typespec = typespec;
    node->elem = elem;
    node->token = token;
    return node;
}

Ast_Call *ast_call_expr(Source_File *f, Token open, Token close, Ast *elem, Array<Ast*> arguments) {
    Ast_Call *node = AST_NEW(f, Ast_Call);
    node->elem = elem;
    node->arguments = arguments;
    node->open = open;
    node->close = close;
    return node;
}

Ast_Selector *ast_selector_expr(Source_File *f, Token token, Ast *parent, Ast_Ident *name) {
    Ast_Selector *node = AST_NEW(f, Ast_Selector);
    node->parent = parent;
    node->name = name;
    node->token = token;
    return node;
}

Ast_Value_Decl *ast_value_decl(Source_File *f, Array<Ast*> names, Ast *typespec, Array<Ast*> values, bool is_mutable) {
    Ast_Value_Decl *node = AST_NEW(f, Ast_Value_Decl);
    node->names = names;
    node->typespec = typespec;
    node->values = values;
    node->is_mutable = is_mutable;
    return node;
}

Ast_Load *ast_load_stmt(Source_File *f, Token token, Token file_token, String file_path) {
    Ast_Load *node = AST_NEW(f, Ast_Load);
    node->rel_path = file_path;
    node->token = token;
    node->file_token = file_token;
    return node;
}

Ast_Import *ast_import_stmt(Source_File *f, Token token, Token file_token, String file_path) {
    Ast_Import *node = AST_NEW(f, Ast_Import);
    node->rel_path = file_path;
    node->token = token;
    node->file_token = file_token;
    return node;
}

Ast_Binary *ast_binary_expr(Source_File *f, Token token, OP op, Ast *lhs, Ast *rhs) {
    Ast_Binary *node = AST_NEW(f, Ast_Binary);
    node->op = op;
    node->lhs = lhs;
    node->rhs = rhs;
    node->token = token;
    return node; 
}

Ast_Assignment *ast_assignment_stmt(Source_File *f, Token token, OP op, Array<Ast*> lhs, Array<Ast*> rhs) {
    Ast_Assignment *node = AST_NEW(f, Ast_Assignment);
    node->op = op;
    node->lhs = lhs;
    node->rhs = rhs;
    node->token = token;
    return node;
}

Ast_Subscript *ast_subscript_expr(Source_File *f, Token open, Token close, Ast *base, Ast *index) {
    Ast_Subscript *node = AST_NEW(f, Ast_Subscript);
    node->expr = base;
    node->index = index;
    node->open = open;
    node->close = close;
    return node;
}

Ast_Sizeof *ast_sizeof_expr(Source_File *f, Token token, Ast *elem) {
    Ast_Sizeof *node = AST_NEW(f, Ast_Sizeof);
    node->elem = elem;
    node->token = token;
    return node;
}

Ast_Expr_Stmt *ast_expr_stmt(Source_File *f, Ast *expr) {
    Ast_Expr_Stmt *node = AST_NEW(f, Ast_Expr_Stmt);
    node->expr = expr;
    return node;
}

Ast_Block *ast_block_stmt(Source_File *f, Token open, Token close, Array<Ast*> statements) {
    Ast_Block *node = AST_NEW(f, Ast_Block);
    node->statements = statements;
    node->open = open;
    node->close = close;
    return node;
}

Ast_If *ast_if_stmt(Source_File *f, Token token, Ast *cond, Ast_Block *block) {
    Ast_If *node = AST_NEW(f, Ast_If);
    node->stmt_flags = STMT_FLAG_PATH_BRANCH;
    node->cond = cond;
    node->block = block;
    node->token = token;
    return node;
}

Ast_Case_Label *ast_case_label(Source_File *f, Token token, Ast *cond, Array<Ast*> statements) {
    Ast_Case_Label *node = AST_NEW(f, Ast_Case_Label);
    node->cond = cond;
    node->statements = statements;
    node->token = token;
    return node;
}

Ast_Ifcase *ast_ifcase_stmt(Source_File *f, Token token, Token open, Token close, Ast *cond, Array<Ast_Case_Label*> clauses, bool check_enum_complete) {
    Ast_Ifcase *node = AST_NEW(f, Ast_Ifcase);
    node->cond = cond;
    node->cases = clauses;
    node->check_enum_complete = check_enum_complete;
    node->token = token;
    node->open = open;
    node->close = close;
    return node;
}

Ast_While *ast_while_stmt(Source_File *f, Token token, Ast *cond, Ast_Block *block) {
    Ast_While *node = AST_NEW(f, Ast_While);
    node->cond = cond;
    node->block = block;
    node->token = token;
    return node;
}

Ast_For *ast_for_stmt(Source_File *f, Token token, Ast *init, Ast *condition, Ast *post, Ast_Block *block) {
    Ast_For *node = AST_NEW(f, Ast_For);
    node->init = init;
    node->condition = condition;
    node->post = post;
    node->block = block;
    node->token = token;
    return node;
}

Ast_Range_Stmt *ast_range_stmt(Source_File *f, Token token, Ast_Assignment *init, Ast_Block *block) {
    Ast_Range_Stmt *node = AST_NEW(f, Ast_Range_Stmt);
    node->init = init;
    node->block = block;
    node->token = token;
    return node;
} 


Ast_Array_Type *ast_array_type(Source_File *f, Token token, Ast *elem, Ast *array_size) {
    Ast_Array_Type *node = AST_NEW(f, Ast_Array_Type);
    node->elem = elem;
    node->array_size = array_size;
    node->token = token;
    return node;
}

Ast_Proc_Type *ast_proc_type(Source_File *f, Token open, Token close, Array<Ast_Param*> params, Array<Ast*> results, bool is_variadic) {
    Ast_Proc_Type *node = AST_NEW(f, Ast_Proc_Type);
    node->params = params;
    node->results = results;
    node->is_variadic = is_variadic;
    node->open = open;
    node->close = close;
    return node;
}

Ast_Enum_Type *ast_enum_type(Source_File *f, Token token, Token open, Token close, Ast *base_type, Array<Ast_Enum_Field*> fields) {
    Ast_Enum_Type *node = AST_NEW(f, Ast_Enum_Type);
    node->base_type = base_type;
    node->fields = fields;
    node->token = token;
    node->open = open;
    node->close = close;
    return node;
}

Ast_Struct_Type *ast_struct_type(Source_File *f, Token token, Token open, Token close, Array<Ast_Value_Decl*> members) {
    Ast_Struct_Type *node = AST_NEW(f, Ast_Struct_Type);
    node->members = members;
    node->token = token;
    node->open = open;
    node->close = close;
    return node;
}

Ast_Union_Type *ast_union_type(Source_File *f, Token token, Token open, Token close, Array<Ast_Value_Decl*> members) {
    Ast_Union_Type *node = AST_NEW(f, Ast_Union_Type);
    node->members = members;
    node->token = token;
    node->open = open;
    node->close = close;
    return node;
}

CString string_from_expr(CString string, Ast *expr) {
    if (expr == nullptr) {
        return string_append(string, "<nil>");
    }

    switch (expr->kind) {
    // case AST_BAD_EXPR:
    case AST_PAREN: {
        Ast_Paren *paren = static_cast<Ast_Paren*>(expr);
        string = string_append(string, "(");
        string = string_from_expr(string, paren->elem);
        string = string_append(string, ")");
        break;
    }

    case AST_LITERAL: {
        Ast_Literal *literal = static_cast<Ast_Literal*>(expr);
        switch (literal->value.kind) {
        case CONSTANT_VALUE_INTEGER:
            string = string_append(string, (char *)string_from_bigint(literal->value.value_integer).data);
            break;
        case CONSTANT_VALUE_FLOAT:
            string = string_append_fmt(string, "%f", literal->value.value_float);
            break;
        case CONSTANT_VALUE_STRING:
            string = string_append_fmt(string, "\"%S\"", literal->value.value_string);
            break;
        }
        break;
    }

    case AST_COMPOUND_LITERAL: {
        Ast_Compound_Literal *literal = static_cast<Ast_Compound_Literal*>(expr);
        break;
    }
        
    case AST_IDENT: {
        Ast_Ident *ident = static_cast<Ast_Ident*>(expr);
        string = string_append(string, (char *)ident->name->data);
        break;
    }

    case AST_CALL: {
        Ast_Call *call = static_cast<Ast_Call*>(expr);
        string = string_from_expr(string, call->elem);
        for (int i = 0; i < call->arguments.count; i++) {
            Ast *arg = call->arguments[i];
            string = string_from_expr(string, arg);
            if (i != call->arguments.count - 1) {
                string = string_append(string, ", ");
            }
        }
        break;
    }

    case AST_SUBSCRIPT: {
        Ast_Subscript *subscript = static_cast<Ast_Subscript*>(expr);
        string = string_from_expr(string, subscript->expr);
        string = string_append(string, "[");
        string = string_from_expr(string, subscript->index);
        string = string_append(string, "]");
        break;
    }

    case AST_CAST: {
        Ast_Cast *cast = static_cast<Ast_Cast*>(expr);
        string = string_append(string, "cast(");
        string = string_from_expr(string, cast->typespec);
        string = string_append(string, ")");
        string = string_from_expr(string, cast->elem);
        break;
    }
    case AST_UNARY: {
        Ast_Unary *unary = static_cast<Ast_Unary*>(expr);
        string = string_append(string, string_from_operator(unary->op));
        string = string_from_expr(string, unary->elem);
        break;
    }

    case AST_STAR_EXPR: {
        Ast_Star_Expr *star = static_cast<Ast_Star_Expr*>(expr);
        string = string_append(string, "*");
        string = string_from_expr(string, star->elem);
        break;
    }
        
    case AST_DEREF: {
        Ast_Deref *deref = static_cast<Ast_Deref*>(expr);
        string = string_append(string, ".*");
        string = string_from_expr(string, deref->elem);
        break;
    }

    case AST_BINARY: {
        Ast_Binary *binary = static_cast<Ast_Binary*>(expr);
        string = string_from_expr(string, binary->lhs);
        string = string_append(string, string_from_operator(binary->op));
        string = string_from_expr(string, binary->rhs);
        break;
    }

    case AST_SELECTOR: {
        Ast_Selector *selector = static_cast<Ast_Selector*>(expr);
        string = string_from_expr(string, selector->parent);
        string = string_append(string, ".");
        string = string_from_expr(string, selector->name);
        break;
    }

    case AST_RANGE: {
        Ast_Range *range = static_cast<Ast_Range*>(expr);
        string = string_from_expr(string, range->lhs);
        string = string_append(string, "..");
        string = string_from_expr(string, range->rhs);
        break;
    }

    case AST_SIZEOF: {
        Ast_Sizeof *size_of = static_cast<Ast_Sizeof*>(expr);
        string = string_append(string, "size_of");
        string = string_from_expr(string, size_of->elem);
        break;
    }
    }

    return string;
}

CString string_from_expr(Allocator allocator, Ast *expr) {
    CString string = cstring_make(allocator, "");
    return string_from_expr(string, expr);
}

CString string_from_expr(Ast *expr) {
    return string_from_expr(heap_allocator(), expr);
}

const char *string_from_ast_kind(Ast_Kind kind) {
    return ast_strings[kind];
}

char *string_from_operator(OP op) {
    switch (op) {
    default:
        Assert(0);
        return "";
    case OP_ADD:
        return "+";

    case OP_SUB:
        return "-";
    case OP_MUL:
        return "*";
    case OP_DIV:
        return "/";
    case OP_MOD:
        return "%";
    case OP_UNARY_MINUS:
        return "-";
    case OP_UNARY_PLUS:
        return "+";
    case OP_SIZEOF:
        return "size_of";
    case OP_EQ:
        return "==";
    case OP_NEQ:
        return "!=";
    case OP_LT:
        return "<";
    case OP_LTEQ:
        return "<=";
    case OP_GT:
        return ">";
    case OP_GTEQ:
        return ">=";
    case OP_NOT:
        return "!";
    case OP_OR:
        return "||";
    case OP_AND:
        return "&&";
    case OP_BIT_NOT:
        return "~";
    case OP_BIT_AND:
        return "&";
    case OP_BIT_OR:
        return "|";
    case OP_XOR:
        return "^";
    case OP_LSH:
        return "<<";
    case OP_RSH:
        return ">>";
    case OP_ASSIGN:
        return "=";
    case OP_ADD_ASSIGN:
        return "+=";
    case OP_SUB_ASSIGN:
        return "-=";
    case OP_MUL_ASSIGN:
        return "*=";
    case OP_DIV_ASSIGN:
        return "/=";
    case OP_MOD_ASSIGN:
        return "%/=";
    case OP_AND_ASSIGN:
        return "&=";
    case OP_OR_ASSIGN:
        return "|=";
    case OP_XOR_ASSIGN:
        return "^=";
    case OP_LSH_ASSIGN:
        return "<<=";
    case OP_RSH_ASSIGN:
        return ">>=";
    case OP_IN:
        return "in";
    case OP_ADDRESS:
        return "*";
    case OP_DEREF:
        return ".*";
    case OP_SUBSCRIPT:
        return "[]";
    case OP_SELECT:
        return ".";
    case OP_CAST:
        return "cast";
    }
}

OP get_unary_operator(Token_Kind kind) {
    switch (kind) {
    default:
        return OP_ERR;
    case TOKEN_CAST:
        return OP_CAST;
    case TOKEN_PLUS:
        return OP_UNARY_PLUS;
    case TOKEN_MINUS:
        return OP_UNARY_MINUS;
    case TOKEN_BANG:
        return OP_NOT;
    case TOKEN_SQUIGGLE:
        return OP_BIT_NOT;
    case TOKEN_SIZEOF:
        return OP_SIZEOF;
    }
}

OP get_binary_operator(Token_Kind kind) {
    switch (kind) {
    default:
        return OP_ERR;

    case TOKEN_PLUS:
        return OP_ADD;
    case TOKEN_MINUS:
        return OP_SUB;
    case TOKEN_STAR:
        return OP_MUL;
    case TOKEN_SLASH:
        return OP_DIV;
    case TOKEN_MOD:
        return OP_MOD;

    case TOKEN_EQ2:
        return OP_EQ;
    case TOKEN_NEQ:
        return OP_NEQ;
    case TOKEN_LT:
        return OP_LT;
    case TOKEN_GT:
        return OP_GT;
    case TOKEN_LTEQ:
        return OP_LTEQ;
    case TOKEN_GTEQ:
        return OP_GTEQ;

    case TOKEN_BAR:
        return OP_BIT_OR;
    case TOKEN_AMPER:
        return OP_BIT_AND;
    case TOKEN_OR:
        return OP_OR;
    case TOKEN_AND:
        return OP_AND;
    case TOKEN_XOR:
        return OP_XOR;

    case TOKEN_LSHIFT:
        return OP_LSH;
    case TOKEN_RSHIFT:
        return OP_RSH;

    case TOKEN_EQ:
        return OP_ASSIGN;
    case TOKEN_IN:
        return OP_IN;
    case TOKEN_PLUS_EQ:
        return OP_ADD_ASSIGN;
    case TOKEN_MINUS_EQ:
        return OP_SUB_ASSIGN;
    case TOKEN_STAR_EQ:
        return OP_MUL_ASSIGN;
    case TOKEN_SLASH_EQ:
        return OP_DIV_ASSIGN;
    case TOKEN_MOD_EQ:
        return OP_MOD_ASSIGN;
    case TOKEN_AMPER_EQ:
        return OP_AND_ASSIGN;
    case TOKEN_BAR_EQ:
        return OP_OR_ASSIGN;
    case TOKEN_XOR_EQ:
        return OP_XOR_ASSIGN;
    case TOKEN_LSHIFT_EQ:
        return OP_LSH_ASSIGN;
    case TOKEN_RSHIFT_EQ:
        return OP_RSH_ASSIGN;
    }
}

int get_operator_precedence(OP op) {
    switch (op) {
    default:
    case OP_ERR:
        return -1;

    case OP_SUBSCRIPT:
    case OP_SELECT:
        return 13000;

    case OP_UNARY_PLUS:
    case OP_UNARY_MINUS:
    case OP_NOT:
    case OP_BIT_NOT:
    case OP_DEREF:
    case OP_ADDRESS:
    case OP_CAST:
    case OP_SIZEOF:
        return 12000;

    case OP_MUL:
    case OP_DIV:
    case OP_MOD:
        return 11000;

    case OP_ADD:
    case OP_SUB:
        return 10000;

    case OP_LT:
    case OP_LTEQ:
    case OP_GT:
    case OP_GTEQ:
        return 9000;

    case OP_EQ:
    case OP_NEQ:
        return 8000;

    case OP_BIT_AND:
        return 70000;

    case OP_XOR:
        return 6000;

    case OP_BIT_OR:
        return 5000;

    case OP_AND:
        return 4000;

    case OP_OR:
        return 3000;

    case OP_LSH:
    case OP_RSH:
        return 2000;

    case OP_ASSIGN:
    case OP_ADD_ASSIGN:
    case OP_SUB_ASSIGN:
    case OP_MUL_ASSIGN:
    case OP_DIV_ASSIGN:
    case OP_MOD_ASSIGN:
    case OP_LSHIFT_ASSIGN:
    case OP_RSHIFT_ASSIGN:
    case OP_AND_ASSIGN:
    case OP_OR_ASSIGN:
    case OP_XOR_ASSIGN:
        return -1;
        // return 1000;
    }
}

