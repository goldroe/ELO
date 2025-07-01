#ifndef AST_H
#define AST_H

struct Type;
struct Ast;
struct Ast_Stmt;
struct Ast_Decl;
struct Ast_Block;
struct Ast;
struct Ast_Ident;
struct Ast_Operator_Proc;
struct Scope;
struct BE_Var;
struct BE_Proc;
struct BE_Struct;
struct Ast_Value_Decl;

struct Decl;

struct Ast_Pointer_Type;
struct Ast_Array_Type;
struct Ast_Proc_Type;
struct Ast_Struct_Type;
struct Ast_Enum_Type;

enum OP {
    OP_ERR = -1,

    OP_ADDRESS,
    OP_DEREF,
    OP_SUBSCRIPT,
    OP_SELECT,
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

#define AST_KINDS \
    AST_KIND(AST_NIL              , "Nil"),                  \
    AST_KIND(AST_ROOT             , "Root"),                \
    AST_KIND(AST_DECL_BEGIN       , "Decl__Begin"),     \
    AST_KIND(AST_DECL             , "Decl"),                \
    AST_KIND(AST_BAD_DECL         , "BadDecl"),         \
    AST_KIND(AST_TYPE_DECL        , "TypeDecl"),       \
    AST_KIND(AST_PARAM            , "Param"),              \
    AST_KIND(AST_VAR              , "Var"),                  \
    AST_KIND(AST_STRUCT           , "Struct"),            \
    AST_KIND(AST_ENUM             , "Enum"),                \
    AST_KIND(AST_ENUM_FIELD       , "Field"),         \
    AST_KIND(AST_PROC             , "Proc"),                    \
    AST_KIND(AST_PROC_LIT         , "ProcLit"),                \
    AST_KIND(AST_OPERATOR_PROC    , "OperatorProc"),           \
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
    AST_KIND(AST_CALL             , "CallExpr"), \
    AST_KIND(AST_SUBSCRIPT        , "SubscriptExpr"), \
    AST_KIND(AST_CAST             , "CastExpr"), \
    AST_KIND(AST_ITERATOR         , "IteratorExpr"), \
    AST_KIND(AST_UNARY            , "UnaryExpr"), \
    AST_KIND(AST_ADDRESS          , "AddressExpr"), \
    AST_KIND(AST_DEREF            , "DerefExpr"), \
    AST_KIND(AST_BINARY           , "BinaryExpr"), \
    AST_KIND(AST_SELECTOR         , "SelectorExpr"), \
    AST_KIND(AST_RANGE            , "RangeExpr"), \
    AST_KIND(AST_SIZEOF           , "SizeofExpr"), \
    AST_KIND(AST_EXPR_END         , "Expr__End"), \
    AST_KIND(AST_TYPE_BEGIN       , "Type__Begin"), \
    AST_KIND(AST_POINTER_TYPE     , "PointerType"), \
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

const char *ast_strings[] = {
#define AST_KIND(K,S) S
    AST_KINDS
#undef AST_KIND
};

#define ast_node_var(N, T, V) T *N = (T *)(V)
#define ast_node(T, V) ((T *)(V))

enum Ast_Flags {
    AST_FLAG_LVALUE   = (1<<0),
    AST_FLAG_CONSTANT = (1<<1),
    AST_FLAG_TYPE     = (1<<2),
    AST_FLAG_OP_CALL  = (1<<3),
    AST_FLAG_GLOBAL   = (1<<4),
};
EnumDefineFlagOperators(Ast_Flags);

enum Addressing_Mode {
    ADDRESSING_INVALID,
    ADDRESSING_TYPE,
    ADDRESSING_VALUE,
    ADDRESSING_VARIABLE,
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
    Type *inferred_type = nullptr;
    Constant_Value value = {};

    Ast_Flags flags = (Ast_Flags)0;

    b32 is_poisoned = false;
    b32 visited = false;

    bool invalid() { return is_poisoned; }
    bool valid() { return !is_poisoned; }
    void poison() { is_poisoned = true; }

    bool is_decl() { return AST_DECL_BEGIN <= kind && kind <= AST_DECL_END; }
    bool is_expr() { return AST_EXPR_BEGIN <= kind && kind <= AST_EXPR_END; }
    bool is_stmt() { return AST_STMT_BEGIN <= kind && kind <= AST_STMT_END; }
};

struct Ast_Root : Ast {
    Ast_Root() { kind = AST_ROOT; }
    Scope *scope = nullptr;
    Auto_Array<Ast*> decls;
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

struct Ast_Compound_Literal : Ast {
    Ast_Compound_Literal() { kind = AST_COMPOUND_LITERAL; }
    Ast *type;
    Auto_Array<Ast*> elements;
    Token open;
    Token close;
};

struct Ast_Call : Ast {
    Ast_Call() { kind = AST_CALL; }
    Ast *elem;
    Auto_Array<Ast*> arguments;
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
    Ast_Operator_Proc *proc;
    Ast *elem;
    Token token;
};

struct Ast_Address : Ast {
    Ast_Address() { kind = AST_ADDRESS; }
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
    Ast *type;
    Ast *elem;
    Token token;
};

struct Ast_Assignment : Ast {
    Ast_Assignment() { kind = AST_ASSIGNMENT; }
    OP op;
    Auto_Array<Ast*> lhs;
    Auto_Array<Ast*> rhs;
    // Ast_Operator_Proc *proc;
    Token token;
};

struct Ast_Binary : Ast {
    Ast_Binary() { kind = AST_BINARY; }
    OP op;
    Ast_Operator_Proc *proc;
    Ast *lhs;
    Ast *rhs;
    Token token;
};

struct Ast_Sizeof : Ast {
    Ast_Sizeof() { kind = AST_SIZEOF; }
    Ast *elem;
    Token token;
    Token open;
    Token close;
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

struct Ast_Type_Decl : Ast_Decl {
    Ast_Type_Decl() { kind = AST_TYPE_DECL; }
    Ast *type;
    Token token;
};

struct Ast_Param : Ast {
    Ast_Param() { kind = AST_PARAM; }
    Ast_Ident *name;
    Ast *type;
    b32 is_vararg;
};

struct Ast_Var : Ast_Decl {
    Ast_Var() { kind = AST_VAR; }
    Ast *type;
    Ast *init;
};

struct Ast_Struct : Ast_Decl {
    Ast_Struct() { kind = AST_STRUCT; }
    Scope *scope;
    Auto_Array<Ast_Decl*> members;
    BE_Struct *backend_struct;
};

struct Ast_Enum : Ast_Decl {
    Ast_Enum() { kind = AST_ENUM; }
    Scope *scope;
    Auto_Array<Ast*> fields;
};

struct Ast_Proc_Lit : Ast {
    Ast_Proc_Lit() { kind = AST_PROC_LIT; }
    Scope *scope;
    Ast_Proc_Type *type;
    Ast_Block *body;

    Proc_Resolve_State proc_resolve_state = PROC_RESOLVE_STATE_UNSTARTED;

    Auto_Array<Ast_Decl*> local_vars;
    b32 returns;
    BE_Proc *backend_proc;
};

struct Ast_Proc : Ast_Decl {
    Ast_Proc() { kind = AST_PROC; }
    Scope *scope = nullptr;

    Auto_Array<Ast_Param*> params;
    Ast *return_type = nullptr;
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
    Token token;
};

struct Ast_Bad_Expr : Ast {
    Ast_Bad_Expr() { kind = AST_BAD_EXPR; }
    Token start;
    Token end;
};

struct Ast_Bad_Stmt : Ast_Stmt {
    Ast_Bad_Stmt() { kind = AST_BAD_STMT; }
    Token start;
    Token end;
};

struct Ast_Bad_Decl : Ast_Decl {
    Ast_Bad_Decl() { kind = AST_BAD_DECL; }
    Token start;
    Token end;
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
    Auto_Array<Ast*> statements;

    Token token;

    b32 is_default;
    b32 fallthrough;

    void *backend_block;
};

struct Ast_Ifcase : Ast_Stmt {
    Ast_Ifcase() { kind = AST_IFCASE; }
    Ast *cond;
    Ast_Case_Label *default_case;
    Auto_Array<Ast_Case_Label*> cases;

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

    //@Todo Regular for loop
    // for init; condition; post { .. }
    // Ast *init;
    // Ast *condition;
    // Ast *post;

    //@Note Range for loop
    // for index, value: expr
    Auto_Array<Ast*> lhs;
    Ast *range_expr;
    Ast_Block *block;

    Decl *index_variable = nullptr;
    Decl *value_variable = nullptr;

    Token token;

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
    String8 rel_path;
    Token token;
    Token file_token;
};

struct Ast_Import : Ast {
    Ast_Import() { kind = AST_IMPORT_STMT; }
    String8 rel_path;
    Token token;
    Token file_token;
};

struct Ast_Block : Ast_Stmt {
    Ast_Block() { kind = AST_BLOCK; }
    Scope *scope;
    Auto_Array<Ast*> statements;
    Token open;
    Token close;

    b32 returns;
};

struct Ast_Return : Ast_Stmt {
    Ast_Return() { kind = AST_RETURN; }
    Auto_Array<Ast*> values;
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
    Auto_Array<Ast*> names;
    Ast *type;
    Auto_Array<Ast*> values;
    bool is_mutable;
};

struct Ast_Pointer_Type : Ast {
    Ast_Pointer_Type() { kind = AST_POINTER_TYPE; }
    Ast *type;
    Token token;
};

struct Ast_Array_Type : Ast {
    Ast_Array_Type() { kind = AST_ARRAY_TYPE; }
    Ast *type;
    Ast *length;
    Token token;
};

struct Ast_Proc_Type : Ast {
    Ast_Proc_Type() { kind = AST_PROC_TYPE; }
    Scope *scope;
    Auto_Array<Ast_Param*> params;
    Auto_Array<Ast*> results;
    Token open;
    Token close;

    b32 foreign;
    b32 variadic;
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
    Auto_Array<Ast_Enum_Field*> fields;
    Token token;
    Token open;
    Token close;
};

struct Ast_Struct_Type : Ast {
    Ast_Struct_Type() { kind = AST_STRUCT_TYPE; }
    Scope *scope;
    Ast_Ident *name;
    Auto_Array<Ast_Value_Decl*> members;
    b32 complete = false;

    Token token;
    Token open;
    Token close;
};

struct Ast_Union_Type : Ast {
    Ast_Union_Type() { kind = AST_UNION_TYPE; }
    Scope *scope;
    Auto_Array<Ast_Value_Decl*> members;

    Token token;
    Token open;
    Token close;
};

#define AST_NEW(F, T) static_cast<T*>(ast__init(&(*ast_alloc(sizeof(T), alignof(T)) = T()), F))

internal inline Allocator ast_allocator();
internal Ast *ast_alloc(Source_File *f, u64 size, int alignment);
internal char *string_from_operator(OP op);

#endif // AST_H
