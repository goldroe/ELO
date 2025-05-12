#ifndef AST_H
#define AST_H

struct Ast;
struct Ast_Expr;
struct Ast_Stmt;
struct Ast_Decl;
struct Ast_Block;
struct Ast_Type_Defn;
struct Ast_Type_Info;
struct Scope;
struct Ast_Ident;
struct Ast_Operator_Proc;

enum Ast_Kind {
    AST_NIL,

    AST_NAME_SCOPE,
    AST_SCOPE,
    AST_ROOT,

    AST_TYPE_DEFN,
    AST_TYPE_INFO,
    AST_ARRAY_TYPE_INFO,
    AST_ENUM_TYPE_INFO,
    AST_PROC_TYPE_INFO,

    AST_EXPR,
    AST_NULL,
    AST_PAREN,
    AST_LITERAL,
    AST_COMPOUND_LITERAL,
    AST_IDENT,
    AST_CALL,
    AST_INDEX,
    AST_CAST,
    AST_ITERATOR,

    AST_UNARY,
    AST_ADDRESS,
    AST_DEREF,
    AST_BINARY,
    AST_ASSIGNMENT,
    AST_FIELD,
    AST_RANGE,
    AST_TERNARY,

    AST_DECL,
    AST_TYPE_DECL,
    AST_PARAM,
    AST_VAR,
    AST_STRUCT,
    AST_ENUM,
    AST_STRUCT_FIELD,
    AST_ENUM_FIELD,

    AST_PROC,
    AST_OPERATOR_PROC,

    AST_STMT,
    AST_EXPR_STMT,
    AST_DECL_STMT,
    AST_IF,
    AST_SWITCH,
    AST_WHILE,
    AST_FOR,
    AST_BLOCK,
    AST_RETURN,
    AST_CONTINUE,
    AST_BREAK,
    AST_GOTO,
    AST_DEFER,

    AST_COUNT
};

struct Ast {
    Ast *parent = NULL;
    Ast_Kind kind = AST_NIL;

    Source_File *file = NULL;
    Source_Pos start = {};
    Source_Pos end = {};

    b32 is_poisoned = 0;
    b32 visited = 0;

    void mark_start(Source_Pos pos) { start = pos; file = pos.file; }
    void mark_end(Source_Pos pos)   { end = pos; file = pos.file; }
    void mark_range(Source_Pos p0, Source_Pos p1) { start = p0; end = p1; file = p0.file; }

    bool invalid() { return is_poisoned; }
    bool valid() { return !is_poisoned; }
    void poison() { is_poisoned = true; }
};

struct Ast_Root : Ast {
    Ast_Root() { kind = AST_ROOT; }
    Auto_Array<Ast_Decl*> declarations;
    Scope *scope = NULL;
};

enum Type_Defn_Kind {
    TYPE_DEFN_NIL,
    TYPE_DEFN_NAME,
    TYPE_DEFN_POINTER,
    TYPE_DEFN_ARRAY,
};

struct Ast_Type_Defn : Ast {
    Ast_Type_Defn() { kind = AST_TYPE_DEFN; }
    Ast_Type_Defn *base;
    Type_Defn_Kind type_defn_kind;
    union {
        Atom *name;
        Ast_Expr *array_size;
    };
};

enum Expr_Flags {
    EXPR_FLAG_NIL         = 0,
    EXPR_FLAG_CONSTANT    = (1<<0),
    EXPR_FLAG_LVALUE      = (1<<1),
    EXPR_FLAG_OP_CALL     = (1<<9),
    EXPR_FLAG_ARITHMETIC  = (1<<10),
    EXPR_FLAG_BOOLEAN     = (1<<11),
    EXPR_FLAG_ASSIGNMENT  = (1<<12),
    EXPR_FLAG_COMPARISON  = (1<<13),
};
EnumDefineFlagOperators(Expr_Flags);

//@Todo Sign, Precision
union Eval {
    s64 int_val;
    f64 float_val;
};

struct Ast_Expr : Ast {
    Ast_Expr() { kind = AST_EXPR; }
    Expr_Flags expr_flags;
    Ast_Type_Info *type_info;
    Eval eval;

    bool inline is_constant() { return expr_flags & EXPR_FLAG_CONSTANT; }
};

struct Ast_Null : Ast_Expr {
    Ast_Null() { kind = AST_NULL; }
};

struct Ast_Paren : Ast_Expr {
    Ast_Paren() { kind = AST_PAREN; }
    Ast_Expr *elem;
};

struct Ast_Index : Ast_Expr {
    Ast_Index() { kind = AST_INDEX; }
    Ast_Expr *lhs;
    Ast_Expr *rhs;
};

struct Ast_Field : Ast_Expr {
    Ast_Field() { kind = AST_FIELD; }
    Ast_Field *field_parent;
    Ast_Field *field_child;
    Ast_Expr *elem;
};

struct Ast_Range : Ast_Expr {
    Ast_Range() { kind = AST_RANGE; }
    Ast_Expr *lhs;
    Ast_Expr *rhs;
};

struct Ast_Literal : Ast_Expr {
    Ast_Literal() { kind = AST_LITERAL; }
    Literal_Flags literal_flags;
    union {
        String8 str_val;
        u64 int_val;
        f64 float_val;
    };
};

struct Ast_Compound_Literal : Ast_Expr {
    Ast_Compound_Literal() { kind = AST_COMPOUND_LITERAL; }
    Ast_Type_Defn *type_defn;
    Auto_Array<Ast_Expr*> elements;
};

struct Ast_Call : Ast_Expr {
    Ast_Call() { kind = AST_CALL; }
    Ast_Expr *elem;
    Auto_Array<Ast_Expr*> arguments;
};

struct Ast_Ident : Ast_Expr {
    Ast_Ident() { kind = AST_IDENT; }
    Atom *name;
    Ast_Decl *reference;
};

struct Ast_Unary : Ast_Expr {
    Ast_Unary() { kind = AST_UNARY; }
    Token op;
    Ast_Operator_Proc *proc;
    Ast_Expr *elem;
};

struct Ast_Address : Ast_Expr {
    Ast_Address() { kind = AST_ADDRESS; }
    Ast_Expr *elem;
};

struct Ast_Deref : Ast_Expr {
    Ast_Deref() { kind = AST_DEREF; }
    Ast_Expr *elem;
};

struct Ast_Cast : Ast_Expr {
    Ast_Cast() { kind = AST_CAST; }
    Ast_Type_Defn *type_defn;
    Ast_Expr *elem;
};

struct Ast_Assignment : Ast_Expr {
    Ast_Assignment() { kind = AST_ASSIGNMENT; }
    Token op;
    Ast_Operator_Proc *proc;
    Ast_Expr *lhs;
    Ast_Expr *rhs;
};

struct Ast_Binary : Ast_Expr {
    Ast_Binary() { kind = AST_BINARY; }
    Token op;
    Ast_Operator_Proc *proc;
    Ast_Expr *lhs;
    Ast_Expr *rhs;
};

enum Resolve_State {
    RESOLVE_UNSTARTED,
    RESOLVE_STARTED,
    RESOLVE_DONE
};

enum Decl_Flags {
    DECL_FLAG_NIL = 0,
    DECL_FLAG_TYPE = (1<<0),
};
EnumDefineFlagOperators(Decl_Flags);

struct Ast_Decl : Ast {
    Ast_Decl() { kind = AST_DECL; }
    Atom *name;
    Decl_Flags decl_flags;
    Ast_Type_Info *type_info;
    Resolve_State resolve_state = RESOLVE_UNSTARTED;
};

struct Ast_Type_Decl : Ast_Decl {
    Ast_Type_Decl() { kind = AST_TYPE_DECL; }
};

struct Ast_Param : Ast_Decl {
    Ast_Param() { kind = AST_PARAM; }
    Ast_Type_Defn *type_defn;
    b32 is_vararg;
};

struct Ast_Var : Ast_Decl {
    Ast_Var() { kind = AST_VAR; }
    Ast_Type_Defn *type_defn;
    Ast_Expr *init;
    int stack_offset;
};

struct Ast_Struct_Field : Ast {
    Ast_Struct_Field() { kind = AST_STRUCT_FIELD; }
    Atom *name;
    Ast_Type_Defn *type_defn;
    Ast_Type_Info *type_info;
};

struct Ast_Struct : Ast_Decl {
    Ast_Struct() { kind = AST_STRUCT; }
    Auto_Array<Ast_Struct_Field*> fields;
};

struct Ast_Enum_Field : Ast {
    Ast_Enum_Field() { kind = AST_ENUM_FIELD; }
    Atom *name;
    s64 value;
};

struct Ast_Enum : Ast_Decl {
    Ast_Enum() { kind = AST_ENUM; }
    Auto_Array<Ast_Enum_Field*> fields;
};

struct Ast_Proc : Ast_Decl {
    Ast_Proc() { kind = AST_PROC; }
    Auto_Array<Ast_Param*> parameters;
    Ast_Type_Defn *return_type_defn = NULL;
    Scope *scope = NULL;
    Ast_Block *block = NULL;
    b32 foreign;
    b32 has_varargs;
    b32 returns;
};

struct Ast_Operator_Proc : Ast_Proc {
    Ast_Operator_Proc() { kind = AST_OPERATOR_PROC; }
    Token_Kind op;
};

enum Stmt_Flags {
    STMT_FLAG_PATH_BRANCH = (1<<8),
};

struct Ast_Stmt : Ast {
    Ast_Stmt() { kind = AST_STMT; }
    Stmt_Flags stmt_flags;
};

struct Ast_If : Ast_Stmt {
    Ast_If() { kind = AST_IF; }
    Ast_Expr *cond;
    Ast_Block *block;
    Ast_If *if_next = NULL;
    Ast_If *if_prev = NULL;
};

struct Ast_While : Ast_Stmt {
    Ast_While() { kind = AST_WHILE; }
    Ast_Expr *cond;
    Ast_Block *block;
};

// struct Ast_Iterator : Ast_Expr {
//     Ast_Iterator() { kind = AST_ITERATOR; }
//     Ast_Ident *ident;
//     Ast_Expr *range;
// };

struct Ast_For : Ast_Stmt {
    Ast_For() { kind = AST_FOR; }
    Ast_Stmt *init;
    Ast_Expr *cond;
    Ast_Expr *iterator;
    Ast_Block *block;
};

struct Ast_Expr_Stmt : Ast_Stmt {
    Ast_Expr_Stmt() { kind = AST_EXPR_STMT; }   
    Ast_Expr *expr;
};

struct Ast_Decl_Stmt : Ast_Stmt {
    Ast_Decl_Stmt() { kind = AST_DECL_STMT; }
    Ast_Decl *decl;
};

struct Ast_Block : Ast_Stmt {
    Ast_Block() { kind = AST_BLOCK; }
    Ast_Block *block_parent = NULL;
    Ast_Block *block_next = NULL;
    Ast_Block *block_prev = NULL;
    Ast_Block *block_first = NULL;
    Ast_Block *block_last = NULL;

    Auto_Array<Ast_Stmt*> statements;
    Scope *scope = NULL;
    b32 returns;
};

struct Ast_Return : Ast_Stmt {
    Ast_Return() { kind = AST_RETURN; }
    Ast_Expr *expr;
};

struct Ast_Break : Ast_Stmt {
    Ast_Break() { kind = AST_BREAK; }
};

struct Ast_Continue : Ast_Stmt {
    Ast_Continue() { kind = AST_CONTINUE; }
};


#define AST_NEW(T) static_cast<T*>(&(*ast_alloc(sizeof(T), alignof(T)) = T()))

internal Ast *ast_alloc(u64 size, int alignment);
internal Ast_Type_Info *ast_pointer_type_info(Ast_Type_Info *base);

#endif // AST_H
