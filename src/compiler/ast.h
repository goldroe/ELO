#ifndef AST_H
#define AST_H

#include "array.h"

#include "base/base_core.h"
#include "base/base_strings.h"

#include "lexer.h"
#include "OP.h"
#include "source_file.h"
#include "constant_value.h"

extern Arena *g_ast_arena;
extern u64 g_ast_counter;


struct Type;
struct Ast;
struct Ast_Stmt;
struct Ast_Decl;
struct Ast_Block;
struct Ast;
struct Ast_Ident;
struct Scope;
struct BE_Var;
struct BE_Proc;
struct BE_Struct;
struct Ast_Value_Decl;

struct Ast_Stmt;

struct Decl;

struct Ast_Array_Type;
struct Ast_Proc_Type;
struct Ast_Struct_Type;
struct Ast_Enum_Type;

#define AST_KINDS \
    AST_KIND(AST_NIL              , "Nil"),                  \
    AST_KIND(AST_ROOT             , "Root"),                \
    AST_KIND(AST_DECL_BEGIN       , "Decl__Begin"),     \
    AST_KIND(AST_DECL             , "Decl"),                \
    AST_KIND(AST_BAD_DECL         , "BadDecl"),         \
    AST_KIND(AST_PARAM            , "Param"),              \
    AST_KIND(AST_ENUM_FIELD       , "Field"),         \
    AST_KIND(AST_PROC_LIT         , "ProcLit"),                \
    AST_KIND(AST_VALUE_DECL       , "ValueDecl"),              \
    AST_KIND(AST_DECL_END         , "Decl__End"),                \
    AST_KIND(AST_STMT_BEGIN       , "Stmt__Begin"), \
    AST_KIND(AST_STMT             , "Stmt"), \
    AST_KIND(AST_EMPTY_STMT       , "EmptyStmt"), \
    AST_KIND(AST_BAD_STMT         , "BadStmt"), \
    AST_KIND(AST_EXPR_STMT        , "ExprStmt"), \
    AST_KIND(AST_DECL_STMT        , "DeclStmt"), \
    AST_KIND(AST_ASSIGNMENT       , "AssignmentStmt"), \
    AST_KIND(AST_IF               , "IfStmt"), \
    AST_KIND(AST_IFCASE           , "IfcaseStmt"), \
    AST_KIND(AST_CASE_LABEL       , "CaseLabel"), \
    AST_KIND(AST_DO_WHILE         , "DoWhileStmt"), \
    AST_KIND(AST_WHILE            , "WhileStmt"), \
    AST_KIND(AST_FOR              , "ForStmt"), \
    AST_KIND(AST_RANGE_STMT       , "RangeStmt"), \
    AST_KIND(AST_BLOCK            , "Block"), \
    AST_KIND(AST_RETURN           , "ReturnStmt"), \
    AST_KIND(AST_CONTINUE         , "ContinueStmt"), \
    AST_KIND(AST_BREAK            , "BreakStmt"), \
    AST_KIND(AST_FALLTHROUGH      , "FallthroughStmt"), \
    AST_KIND(AST_LOAD_STMT        , "LoadStmt"), \
    AST_KIND(AST_IMPORT_STMT      , "ImportStmt"), \
    AST_KIND(AST_DEFER_STMT       , "DeferStmt"), \
    AST_KIND(AST_STMT_END         , "Stmt__End"), \
    AST_KIND(AST_EXPR_BEGIN       , "Expr__Begin"), \
    AST_KIND(AST_EXPR             , "Expr"), \
    AST_KIND(AST_BAD_EXPR         , "BadExpr"), \
    AST_KIND(AST_PAREN            , "ParenExpr"), \
    AST_KIND(AST_LITERAL          , "LiteralExpr"), \
    AST_KIND(AST_COMPOUND_LITERAL , "CompoundLiteralExpr"), \
    AST_KIND(AST_IDENT            , "IdentExpr"), \
    AST_KIND(AST_UNINIT           , "UninitExpor"), \
    AST_KIND(AST_CALL             , "CallExpr"), \
    AST_KIND(AST_SUBSCRIPT        , "SubscriptExpr"), \
    AST_KIND(AST_CAST             , "CastExpr"), \
    AST_KIND(AST_UNARY            , "UnaryExpr"), \
    AST_KIND(AST_STAR_EXPR        , "StarExpr"), \
    AST_KIND(AST_DEREF            , "DerefExpr"), \
    AST_KIND(AST_BINARY           , "BinaryExpr"), \
    AST_KIND(AST_SELECTOR         , "SelectorExpr"), \
    AST_KIND(AST_RANGE            , "RangeExpr"), \
    AST_KIND(AST_SIZEOF           , "SizeofExpr"), \
    AST_KIND(AST_EXPR_END         , "Expr__End"), \
    AST_KIND(AST_TYPE_BEGIN       , "Type__Begin"), \
    AST_KIND(AST_ARRAY_TYPE       , "ArrayType"), \
    AST_KIND(AST_PROC_TYPE        , "ProcType"), \
    AST_KIND(AST_ENUM_TYPE        , "EnumType"), \
    AST_KIND(AST_STRUCT_TYPE      , "StructType"), \
    AST_KIND(AST_UNION_TYPE       , "UnionType"), \
    AST_KIND(AST_TYPE_END         , "Type__End"), \

enum Ast_Kind {
#define AST_KIND(K,S) K
    AST_KINDS
#undef AST_KIND
    AST_COUNT
};

extern const char *ast_strings[];

#define ast_node_var(N, T, V) T *N = (T *)(V)
#define ast_node(T, V) ((T *)(V))

enum Addressing_Mode {
    ADDRESSING_INVALID,
    ADDRESSING_TYPE,
    ADDRESSING_VALUE,    // rvalue
    ADDRESSING_VARIABLE, // lvalue
    ADDRESSING_CONSTANT,
    ADDRESSING_PROCEDURE,
};

struct Ast {
    u64 id = 0;
    Ast_Kind kind = AST_NIL;
    Ast *next = nullptr;
    Ast *prev = nullptr;

    Source_File *file = nullptr;

    Addressing_Mode mode = ADDRESSING_INVALID;
    Type *type = nullptr;
    Constant_Value value = {};

    b32 is_poisoned = false;
    b32 visited = false;

    bool invalid() { return is_poisoned; }
    bool valid() { return !is_poisoned; }
    void poison() { is_poisoned = true; }
};

struct Ast_Root : Ast {
    Ast_Root() { kind = AST_ROOT; }
    Scope *scope = nullptr;
    Array<Ast*> decls;
};

struct Ast_Bad_Expr : Ast {
    Ast_Bad_Expr() { kind = AST_BAD_EXPR; }
    Token start;
    Token end;
};

struct Ast_Paren : Ast {
    Ast_Paren() { kind = AST_PAREN; }
    Ast *elem;
    Token open;
    Token close;
};

struct Ast_Subscript : Ast {
    Ast_Subscript() { kind = AST_SUBSCRIPT; }
    Ast *expr;
    Ast *index;
    Token open;
    Token close;
};

struct Ast_Selector : Ast {
    Ast_Selector() { kind = AST_SELECTOR; }
    Ast *parent;
    Ast_Ident *name;
    Token token;
};

struct Ast_Range : Ast {
    Ast_Range() { kind = AST_RANGE; }
    Ast *lhs;
    Ast *rhs;
    Token token;
};

struct Ast_Literal : Ast {
    Ast_Literal() { kind = AST_LITERAL; }
    Token token;
};

struct Ast_Uninit : Ast {
    Ast_Uninit() { kind = AST_UNINIT; }
    Token token;
};

struct Ast_Compound_Literal : Ast {
    Ast_Compound_Literal() { kind = AST_COMPOUND_LITERAL; }
    Ast *typespec;
    Array<Ast*> elements;
    Token open;
    Token close;
};

struct Ast_Call : Ast {
    Ast_Call() { kind = AST_CALL; }
    Ast *elem;
    Array<Ast*> arguments;
    Token open;
    Token close;
};

struct Ast_Ident : Ast {
    Ast_Ident() { kind = AST_IDENT; }
    Atom *name;
    Token token;
    Decl *ref;
};

struct Ast_Unary : Ast {
    Ast_Unary() { kind = AST_UNARY; }
    OP op;
    Ast *elem;
    Token token;
};

struct Ast_Star_Expr : Ast {
    Ast_Star_Expr() { kind = AST_STAR_EXPR; }
    Ast *elem;
    Token token;
};

struct Ast_Deref : Ast {
    Ast_Deref() { kind = AST_DEREF; }
    Ast *elem;
    Token token;
};

struct Ast_Cast : Ast {
    Ast_Cast() { kind = AST_CAST; }
    Ast *typespec;
    Ast *elem;
    Token token;
};

struct Ast_Binary : Ast {
    Ast_Binary() { kind = AST_BINARY; }
    OP op;
    Ast *lhs;
    Ast *rhs;
    Token token;
};

struct Ast_Sizeof : Ast {
    Ast_Sizeof() { kind = AST_SIZEOF; }
    Ast *elem;
    Token token;
};

enum Resolve_State {
    RESOLVE_UNSTARTED,
    RESOLVE_STARTED,
    RESOLVE_DONE
};

enum Proc_Resolve_State {
    PROC_RESOLVE_STATE_UNSTARTED,
    PROC_RESOLVE_STATE_HEADER,
    PROC_RESOLVE_STATE_COMPLETE,
};

enum Decl_Flags {
    DECL_FLAG_NIL = 0,
    DECL_FLAG_TYPE = (1<<0),
    DECL_FLAG_CONST = (1<<1),
    DECL_FLAG_GLOBAL = (1<<2),
};
EnumDefineFlagOperators(Decl_Flags);

struct Ast_Decl : Ast {
    Ast_Decl() { kind = AST_DECL; }
    Atom *name;
    Decl_Flags decl_flags;
    Resolve_State resolve_state = RESOLVE_UNSTARTED;
    Type *type;
    BE_Var *backend_var;
};

struct Ast_Bad_Decl : Ast_Decl {
    Ast_Bad_Decl() { kind = AST_BAD_DECL; }
    Token start;
    Token end;
};

struct Ast_Param : Ast {
    Ast_Param() { kind = AST_PARAM; }
    Ast_Ident *name;
    Ast *typespec;
    b32 is_variadic;
};

struct Ast_Proc_Lit : Ast {
    Ast_Proc_Lit() { kind = AST_PROC_LIT; }
    Scope *scope;
    Ast_Proc_Type *typespec;
    Ast_Block *body;

    Proc_Resolve_State proc_resolve_state = PROC_RESOLVE_STATE_UNSTARTED;

    Array<Ast*> local_vars;
    b32 returns;
    BE_Proc *backend_proc;
};

enum Stmt_Flags {
    STMT_FLAG_PATH_BRANCH = (1<<8),
};

struct Ast_Stmt : Ast {
    Ast_Stmt() { kind = AST_STMT; }
    Stmt_Flags stmt_flags;
};

struct Ast_Empty_Stmt : Ast_Stmt {
    Ast_Empty_Stmt() { kind = AST_EMPTY_STMT; }
    Token token;
};

struct Ast_Bad_Stmt : Ast_Stmt {
    Ast_Bad_Stmt() { kind = AST_BAD_STMT; }
    Token start;
    Token end;
};

struct Ast_Assignment : Ast_Stmt {
    Ast_Assignment() { kind = AST_ASSIGNMENT; }
    OP op;
    Array<Ast*> lhs;
    Array<Ast*> rhs;
    Token token;
};

struct Ast_If : Ast_Stmt {
    Ast_If() { kind = AST_IF; }
    Ast *cond;
    Ast_Block *block;
    b32 is_else;
    Token token;
};

struct Ast_Case_Label : Ast_Stmt {
    Ast_Case_Label() { kind = AST_CASE_LABEL; }
    Scope *scope;
    Ast *cond;
    Array<Ast*> statements;

    Token token;

    b32 is_default;
    b32 fallthrough;

    void *backend_block;
};

struct Ast_Ifcase : Ast_Stmt {
    Ast_Ifcase() { kind = AST_IFCASE; }
    Ast *cond;
    Ast_Case_Label *default_case;
    Array<Ast_Case_Label*> cases;

    Token token;
    Token open;
    Token close;

    b32 switchy;
    b32 check_enum_complete;

    void *exit_block;
};

struct Ast_While : Ast_Stmt {
    Ast_While() { kind = AST_WHILE; }
    Ast *cond;
    Ast_Block *block;

    Token token;

    void *entry_block;
    void *exit_block;
};

struct Ast_Do_While : Ast_Stmt {
    Ast_Do_While() { kind = AST_DO_WHILE; }
    Ast *cond;
    Ast_Block *block;

    Token token;

    // void *entry_block;
    // void *exit_block;
};

struct Ast_For : Ast_Stmt {
    Ast_For() { kind = AST_FOR; }

    Ast *init;
    Ast *condition;
    Ast *post;
    Ast_Block *block;

    Token token;

    void *entry_block;
    void *retry_block;
    void *exit_block;
};

struct Ast_Range_Stmt : Ast_Stmt {
    Ast_Range_Stmt() { kind = AST_RANGE_STMT; }
    Ast_Assignment *init;
    Ast_Block *block;

    Token token;

    Decl *index = nullptr;
    Decl *value = nullptr;

    void *entry_block;
    void *retry_block;
    void *exit_block;
};

struct Ast_Expr_Stmt : Ast_Stmt {
    Ast_Expr_Stmt() { kind = AST_EXPR_STMT; }   
    Ast *expr;
};

struct Ast_Load : Ast {
    Ast_Load() { kind = AST_LOAD_STMT; }
    String rel_path;
    Token token;
    Token file_token;
};

struct Ast_Import : Ast {
    Ast_Import() { kind = AST_IMPORT_STMT; }
    String rel_path;
    Token token;
    Token file_token;
};

struct Ast_Block : Ast_Stmt {
    Ast_Block() { kind = AST_BLOCK; }
    Scope *scope;
    Array<Ast*> statements;
    Token open;
    Token close;

    b32 returns;
};

struct Ast_Return : Ast_Stmt {
    Ast_Return() { kind = AST_RETURN; }
    Array<Ast*> values;
    Token token;
};

struct Ast_Break : Ast_Stmt {
    Ast_Break() { kind = AST_BREAK; }
    Ast *target = nullptr;
    Token token;
};

struct Ast_Continue : Ast_Stmt {
    Ast_Continue() { kind = AST_CONTINUE; }
    Ast *target = nullptr;
    Token token;
};

struct Ast_Fallthrough : Ast_Stmt {
    Ast_Fallthrough() { kind = AST_FALLTHROUGH; }
    Ast *target = nullptr;
    Token token;
};

struct Ast_Defer : Ast_Stmt {
    Ast_Defer() { kind = AST_DEFER_STMT; }
    Ast *stmt;
    Token token;
};

struct Ast_Value_Decl : Ast {
    Ast_Value_Decl() { kind = AST_VALUE_DECL; }
    Array<Ast*> names;
    Ast *typespec;
    Array<Ast*> values;
    bool is_mutable;
};

struct Ast_Array_Type : Ast {
    Ast_Array_Type() { kind = AST_ARRAY_TYPE; }
    Ast *elem;
    Ast *array_size;
    bool is_dynamic;
    bool is_view;
    Token token;
};

struct Ast_Proc_Type : Ast {
    Ast_Proc_Type() { kind = AST_PROC_TYPE; }
    Scope *scope;
    Array<Ast_Param*> params;
    Array<Ast*> results;
    Token open;
    Token close;

    b32 is_foreign;
    b32 is_variadic;
};

struct Ast_Enum_Field : Ast {
    Ast_Enum_Field() { kind = AST_ENUM_FIELD; }
    Ast_Ident *name;
    Ast *expr;
    Token token;
};

struct Ast_Enum_Type : Ast {
    Ast_Enum_Type () { kind = AST_ENUM_TYPE; }
    Scope *scope;
    Ast *base_type;
    Array<Ast_Enum_Field*> fields;

    Token token;
    Token open;
    Token close;
};

struct Ast_Struct_Type : Ast {
    Ast_Struct_Type() { kind = AST_STRUCT_TYPE; }
    Scope *scope;
    Ast_Ident *name;
    Array<Ast_Value_Decl*> members;

    Token token;
    Token open;
    Token close;
};

struct Ast_Union_Type : Ast {
    Ast_Union_Type() { kind = AST_UNION_TYPE; }
    Scope *scope;
    Array<Ast_Value_Decl*> members;

    Token token;
    Token open;
    Token close;
};

#define AST_NEW(F, T) static_cast<T*>(ast__init(&(*ast_alloc(sizeof(T), alignof(T)) = T()), F))

inline Allocator ast_allocator();
inline Ast *ast__init(Ast *node, Source_File *f);
inline Ast *ast_alloc(u64 size, int alignment);

inline Allocator ast_allocator();
Ast *ast_alloc(Source_File *f, u64 size, int alignment);

bool is_ast_type(Ast *node);
bool is_ast_stmt(Ast *node);

inline bool is_assignment_op(OP op) {
    return OP_ASSIGN <= op && op <= OP_ASSIGN_END;
}

inline bool is_binop(Ast *expr, OP op) {
    return expr->kind == AST_BINARY && ((Ast_Binary *)expr)->op == op;
}

Ast_Root *ast_root(Source_File *f, Array<Ast*> decls);
Ast_Empty_Stmt *ast_empty_stmt(Source_File *f, Token token);
Ast_Bad_Expr *ast_bad_expr(Source_File *f, Token start, Token end);
Ast_Bad_Stmt *ast_bad_stmt(Source_File *f, Token start, Token end);
Ast_Bad_Decl *ast_bad_decl(Source_File *f, Token start, Token end);
Ast_Return *ast_return_stmt(Source_File *f, Token token, Array<Ast*> values);
Ast_Continue *ast_continue_stmt(Source_File *f, Token token);
Ast_Break *ast_break_stmt(Source_File *f, Token token);
Ast_Fallthrough *ast_fallthrough_stmt(Source_File *f, Token token);
Ast_Defer *ast_defer_stmt(Source_File *f, Token token, Ast *stmt);
Ast_Proc_Lit *ast_proc_lit(Source_File *f, Ast_Proc_Type *typespec, Ast_Block *body);
Ast_Enum_Field *ast_enum_field(Source_File *f, Ast_Ident *ident, Ast *expr);
Ast_Param *ast_param(Source_File *f, Ast_Ident *name, Ast *typespec, bool is_variadic);
Ast_Ident *ast_ident(Source_File *f, Token token);
Ast_Literal *ast_literal(Source_File *f, Token token);
Ast_Compound_Literal *ast_compound_literal(Source_File *f, Token open, Token close, Ast *typespec, Array<Ast*> elements);
Ast_Uninit *ast_uninit_expr(Source_File *f, Token token);
Ast_Paren *ast_paren_expr(Source_File *f, Token open, Token close, Ast *elem);
Ast_Unary *ast_unary_expr(Source_File *f, Token token, OP op, Ast *elem);
Ast_Star_Expr *ast_star_expr(Source_File *f, Token token, Ast *elem);
Ast_Range *ast_range_expr(Source_File *f, Token token, Ast *lhs, Ast *rhs);
Ast_Deref *ast_deref_expr(Source_File *f, Token token, Ast *elem);
Ast_Cast *ast_cast_expr(Source_File *f, Token token, Ast *typespec, Ast *elem);
Ast_Call *ast_call_expr(Source_File *f, Token open, Token close, Ast *elem, Array<Ast*> arguments);
Ast_Selector *ast_selector_expr(Source_File *f, Token token, Ast *parent, Ast_Ident *name);
Ast_Value_Decl *ast_value_decl(Source_File *f, Array<Ast*> names, Ast *typespec, Array<Ast*> values, bool is_mutable);
Ast_Load *ast_load_stmt(Source_File *f, Token token, Token file_token, String file_path);
Ast_Import *ast_import_stmt(Source_File *f, Token token, Token file_token, String file_path);
Ast_Binary *ast_binary_expr(Source_File *f, Token token, OP op, Ast *lhs, Ast *rhs);
Ast_Assignment *ast_assignment_stmt(Source_File *f, Token token, OP op, Array<Ast*> lhs, Array<Ast*> rhs);
Ast_Subscript *ast_subscript_expr(Source_File *f, Token open, Token close, Ast *base, Ast *index);
Ast_Sizeof *ast_sizeof_expr(Source_File *f, Token token, Ast *elem);
Ast_Expr_Stmt *ast_expr_stmt(Source_File *f, Ast *expr);
Ast_Block *ast_block_stmt(Source_File *f, Token open, Token close, Array<Ast*> statements);
Ast_If *ast_if_stmt(Source_File *f, Token token, Ast *cond, Ast_Block *block);
Ast_Case_Label *ast_case_label(Source_File *f, Token token, Ast *cond, Array<Ast*> statements);
Ast_Ifcase *ast_ifcase_stmt(Source_File *f, Token token, Token open, Token close, Ast *cond, Array<Ast_Case_Label*> clauses, bool check_enum_complete);
Ast_While *ast_while_stmt(Source_File *f, Token token, Ast *cond, Ast_Block *block);
Ast_For *ast_for_stmt(Source_File *f, Token token, Ast *init, Ast *condition, Ast *post, Ast_Block *block);
Ast_Range_Stmt *ast_range_stmt(Source_File *f, Token token, Ast_Assignment *init, Ast_Block *block) ;
Ast_Array_Type *ast_array_type(Source_File *f, Token token, Ast *elem, Ast *array_size);
Ast_Proc_Type *ast_proc_type(Source_File *f, Token open, Token close, Array<Ast_Param*> params, Array<Ast*> results, bool is_variadic);
Ast_Enum_Type *ast_enum_type(Source_File *f, Token token, Token open, Token close, Ast *base_type, Array<Ast_Enum_Field*> fields);
Ast_Struct_Type *ast_struct_type(Source_File *f, Token token, Token open, Token close, Array<Ast_Value_Decl*> members);
Ast_Union_Type *ast_union_type(Source_File *f, Token token, Token open, Token close, Array<Ast_Value_Decl*> members);

CString string_from_expr(CString string, Ast *expr);
CString string_from_expr(Allocator allocator, Ast *expr);
CString string_from_expr(Ast *expr);
const char *string_from_ast_kind(Ast_Kind kind);

char *string_from_operator(OP op);
OP get_unary_operator(Token_Kind kind);
OP get_binary_operator(Token_Kind kind);
int get_operator_precedence(OP op);

#endif // AST_H
