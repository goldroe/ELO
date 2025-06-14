#ifndef AST_H
#define AST_H

struct Type;
struct Ast;
struct Ast_Expr;
struct Ast_Stmt;
struct Ast_Decl;
struct Ast_Block;
struct Ast_Type_Defn;
struct Ast_Ident;
struct Ast_Operator_Proc;
struct Scope;
struct BE_Var;
struct BE_Proc;
struct BE_Struct;

enum OP {
    OP_ERR = -1,

    OP_ADDRESS,
    OP_DEREF,
    OP_SUBSCRIPT,
    OP_ACCESS,
    OP_CAST,

    // Unary
    OP_UNARY_PLUS,
    OP_UNARY_MINUS,
    OP_NOT,
    OP_BIT_NOT,

    // Binary
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,

    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_LTEQ,
    OP_GT,
    OP_GTEQ,

    OP_OR,
    OP_AND,
    OP_BIT_AND,
    OP_BIT_OR,
    OP_XOR,
    OP_LSH,
    OP_RSH,

    // Assign
    OP_ASSIGN,
    OP_ADD_ASSIGN,
    OP_SUB_ASSIGN,
    OP_MUL_ASSIGN,
    OP_DIV_ASSIGN,
    OP_MOD_ASSIGN,
    OP_LSHIFT_ASSIGN,
    OP_RSHIFT_ASSIGN,
    OP_AND_ASSIGN,
    OP_OR_ASSIGN,
    OP_XOR_ASSIGN,
    OP_LSH_ASSIGN,
    OP_RSH_ASSIGN,
    OP_ASSIGN_END,

};

enum Ast_Kind {
    AST_NIL,

    AST_ROOT,
    AST_SCOPE,

    AST_TYPE_DEFN,
    AST_TYPE,
    AST_ARRAY_TYPE,
    AST_ENUM_TYPE,
    AST_PROC_TYPE,


    //@Note Exprs
    AST_EXPR_BEGIN,

    AST_EXPR,
    AST_NULL,
    AST_PAREN,
    AST_LITERAL,
    AST_COMPOUND_LITERAL,
    AST_IDENT,
    AST_CALL,
    AST_SUBSCRIPT,
    AST_CAST,
    AST_ITERATOR,

    AST_UNARY,
    AST_ADDRESS,
    AST_DEREF,
    AST_BINARY,
    AST_ASSIGNMENT,
    AST_ACCESS,
    AST_RANGE,
    AST_SIZEOF,

    AST_EXPR_END,


    //@Note Decls

    AST_DECL_BEGIN,

    AST_DECL,
    AST_BAD_DECL,
    AST_TYPE_DECL,
    AST_PARAM,
    AST_VAR,
    AST_STRUCT,
    AST_UNION,
    AST_ENUM,
    // AST_STRUCT_FIELD,
    AST_ENUM_FIELD,
    AST_PROC,
    AST_OPERATOR_PROC,

    AST_DECL_END,


    //@Note Stmts

    AST_STMT_BEGIN,

    AST_STMT,
    AST_EMPTY_STMT,
    AST_BAD_STMT,
    AST_EXPR_STMT,
    AST_DECL_STMT,
    AST_IF,
    AST_IFCASE,
    AST_CASE_LABEL,
    AST_DO_WHILE,
    AST_WHILE,
    AST_FOR,
    AST_BLOCK,
    AST_RETURN,
    AST_CONTINUE,
    AST_BREAK,
    AST_FALLTHROUGH,
    AST_LOAD_STMT,
    AST_IMPORT_STMT,

    AST_STMT_END,

    AST_COUNT
};

struct Ast {
    Ast_Kind kind = AST_NIL;
    Ast *next = nullptr;
    Ast *prev = nullptr;

    Source_File *file = nullptr;
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

    bool is_decl() { return AST_DECL_BEGIN <= kind && kind <= AST_DECL_END; }
    bool is_expr() { return AST_EXPR_BEGIN <= kind && kind <= AST_EXPR_END; }
    bool is_stmt() { return AST_STMT_BEGIN <= kind && kind <= AST_STMT_END; }
};

struct Ast_Root : Ast {
    Ast_Root() { kind = AST_ROOT; }
    Auto_Array<Ast_Decl*> declarations;
    Scope *scope = nullptr;
};

enum Type_Defn_Kind {
    TYPE_DEFN_NIL,
    TYPE_DEFN_NAME,
    TYPE_DEFN_POINTER,
    TYPE_DEFN_ARRAY,
    TYPE_DEFN_PROC,
    TYPE_DEFN_STRUCT,
};

struct Ast_Type_Defn : Ast {
    Ast_Type_Defn() { kind = AST_TYPE_DEFN; }
    Ast_Type_Defn *base;
    Type_Defn_Kind type_defn_kind;
    union {
        Atom *name;
        Ast_Expr *array_size;
        struct {
            Auto_Array<Ast_Type_Defn*> parameters;
            Ast_Type_Defn *return_type;
        } proc;
    };
};

enum Expr_Flags {
    EXPR_FLAG_NIL         = 0,
    EXPR_FLAG_CONSTANT    = (1<<0),
    EXPR_FLAG_LVALUE      = (1<<1),
    EXPR_FLAG_OP_CALL     = (1<<9),
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
    Type *type;
    Eval eval;

    bool inline is_constant() { return expr_flags & EXPR_FLAG_CONSTANT; }
    inline bool is_binop(OP op);
};

struct Ast_Null : Ast_Expr {
    Ast_Null() { kind = AST_NULL; }
};

struct Ast_Paren : Ast_Expr {
    Ast_Paren() { kind = AST_PAREN; }
    Ast_Expr *elem;
};

struct Ast_Subscript : Ast_Expr {
    Ast_Subscript() { kind = AST_SUBSCRIPT; }
    Ast_Expr *expr;
    Ast_Expr *index;
};

struct Ast_Access : Ast_Expr {
    Ast_Access() { kind = AST_ACCESS; }
    Ast_Expr *parent;
    Ast_Ident *name;
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
    Ast_Decl *ref;
};

struct Ast_Unary : Ast_Expr {
    Ast_Unary() { kind = AST_UNARY; }
    OP op;
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
    OP op;
    Ast_Operator_Proc *proc;
    Ast_Expr *lhs;
    Ast_Expr *rhs;
};

struct Ast_Binary : Ast_Expr {
    Ast_Binary() { kind = AST_BINARY; }
    OP op;
    Ast_Operator_Proc *proc;
    Ast_Expr *lhs;
    Ast_Expr *rhs;
};

struct Ast_Sizeof : Ast_Expr {
    Ast_Sizeof() { kind = AST_SIZEOF; }
    Ast_Expr *elem;
};

enum Resolve_State {
    RESOLVE_UNSTARTED,
    RESOLVE_STARTED,
    RESOLVE_DONE
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

struct Ast_Type_Decl : Ast_Decl {
    Ast_Type_Decl() { kind = AST_TYPE_DECL; }
    Ast_Type_Defn *type_defn;
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
};

struct Ast_Union : Ast_Decl {
    Ast_Union() { kind = AST_UNION; }
};

// struct Ast_Struct_Field : Ast {
//     Ast_Struct_Field() { kind = AST_STRUCT_FIELD; }
//     Atom *name;
//     Ast_Type_Defn *type_defn;
//     Type *type;
// };

struct Ast_Struct : Ast_Decl {
    Ast_Struct() { kind = AST_STRUCT; }
    Scope *scope;
    Auto_Array<Ast_Decl*> members;
    BE_Struct *backend_struct;
};

struct Ast_Enum_Field : Ast_Decl {
    Ast_Enum_Field() { kind = AST_ENUM_FIELD; }
    Ast_Expr *expr;
    s64 value;
};

struct Ast_Enum : Ast_Decl {
    Ast_Enum() { kind = AST_ENUM; }
    Scope *scope;
    Auto_Array<Ast_Enum_Field*> fields;
};

struct Ast_Proc : Ast_Decl {
    Ast_Proc() { kind = AST_PROC; }
    Auto_Array<Ast_Param*> parameters;
    Ast_Type_Defn *return_type_defn = nullptr;
    Scope *scope = nullptr;
    Ast_Block *block = nullptr;

    Auto_Array<Ast_Decl*> local_vars;

    b32 foreign;
    b32 has_varargs;
    b32 returns;

    BE_Proc *backend_proc;
};

struct Ast_Operator_Proc : Ast_Proc {
    Ast_Operator_Proc() { kind = AST_OPERATOR_PROC; }
    OP op;
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
};

struct Ast_Bad_Stmt : Ast_Stmt {
    Ast_Bad_Stmt() { kind = AST_BAD_STMT; }
};


struct Ast_Bad_Decl : Ast_Decl {
    Ast_Bad_Decl() { kind = AST_BAD_DECL; }
};

struct Ast_If : Ast_Stmt {
    Ast_If() { kind = AST_IF; }
    Ast_Expr *cond;
    Ast_Block *block;
    b32 is_else;
};

struct Ast_Case_Label : Ast_Stmt {
    Ast_Case_Label() { kind = AST_CASE_LABEL; }
    Scope *scope;
    Ast_Expr *cond;
    Ast_Block *block;

    b32 is_default;
    b32 fallthrough;

    void *backend_block;
};

struct Ast_Ifcase : Ast_Stmt {
    Ast_Ifcase() { kind = AST_IFCASE; }
    Ast_Expr *cond;
    Ast_Case_Label *default_case;
    Auto_Array<Ast_Case_Label*> cases;
    b32 switchy;
    b32 check_enum_complete;

    void *exit_block;
};

struct Ast_While : Ast_Stmt {
    Ast_While() { kind = AST_WHILE; }
    Ast_Expr *cond;
    Ast_Block *block;

    void *entry_block;
    void *exit_block;
};

struct Ast_Do_While : Ast_Stmt {
    Ast_Do_While() { kind = AST_DO_WHILE; }
    Ast_Expr *cond;
    Ast_Block *block;
    // void *entry_block;
    // void *exit_block;
};

struct Ast_For : Ast_Stmt {
    Ast_For() { kind = AST_FOR; }
    Ast_Var *var;
    Ast_Expr *iterator;
    Ast_Block *block;

    void *entry_block;
    void *retry_block;
    void *exit_block;
};

struct Ast_Expr_Stmt : Ast_Stmt {
    Ast_Expr_Stmt() { kind = AST_EXPR_STMT; }   
    Ast_Expr *expr;
};

struct Ast_Decl_Stmt : Ast_Stmt {
    Ast_Decl_Stmt() { kind = AST_DECL_STMT; }
    Ast_Decl *decl;
};

struct Ast_Load : Ast_Stmt {
    Ast_Load() { kind = AST_LOAD_STMT; }
    String8 rel_path;
};

struct Ast_Import : Ast_Stmt {
    Ast_Import() { kind = AST_IMPORT_STMT; }
    String8 rel_path;
};

struct Ast_Block : Ast_Stmt {
    Ast_Block() { kind = AST_BLOCK; }
    Auto_Array<Ast*> statements;
    Scope *scope = nullptr;
    b32 returns;
};

struct Ast_Return : Ast_Stmt {
    Ast_Return() { kind = AST_RETURN; }
    Ast_Expr *expr = nullptr;
};

struct Ast_Break : Ast_Stmt {
    Ast_Break() { kind = AST_BREAK; }
    Ast *target = nullptr;
};

struct Ast_Continue : Ast_Stmt {
    Ast_Continue() { kind = AST_CONTINUE; }
    Ast *target = nullptr;
};

struct Ast_Fallthrough : Ast_Stmt {
    Ast_Fallthrough() { kind = AST_FALLTHROUGH; }
    Ast *target = nullptr;
};

#define AST_NEW(T) static_cast<T*>(&(*ast_alloc(sizeof(T), alignof(T)) = T()))

internal inline Allocator ast_allocator();
internal Ast *ast_alloc(u64 size, int alignment);
internal char *string_from_operator(OP op);

#endif // AST_H
