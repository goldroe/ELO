#ifndef LLVM_BACKEND_H
#define LLVM_BACKEND_H

#include <llvm-c/Config/llvm-config.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Object.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/DebugInfo.h>
#include <llvm-c/Transforms/PassBuilder.h>

struct LB_Decl;

struct LB_Value {
    LLVMValueRef value;
    LLVMTypeRef type;
};

enum LB_Addr_Kind {
    LB_ADDR_NIL,
    LB_ADDR_COUNT
};

struct LB_Addr {
    LB_Addr_Kind kind;

    LLVMValueRef value;
};

enum LB_Decl_Kind {
    LB_DECL_VAR,
    LB_DECL_PROC,
    LB_DECL_STRUCT
};

struct LB_Decl {
    LB_Decl_Kind kind;
};

struct LB_Struct : LB_Decl {
    Atom *name;
    Ast_Struct *decl;

    LLVMTypeRef type;
    Auto_Array<LLVMTypeRef> element_types;
};

struct LB_Var : LB_Decl {
    Atom *name;
    Ast_Decl *decl;
    LLVMTypeRef type;
    LLVMValueRef alloca;
};

struct LB_Procedure : LB_Decl {
    Atom *name;
    Ast_Proc *proc;

    LLVMValueRef value;
    LLVMTypeRef type;
    Auto_Array<LLVMTypeRef> parameter_types;
    LLVMTypeRef return_type;

    LLVMBuilderRef builder;
    LLVMBasicBlockRef entry;

    Auto_Array<LB_Var*> named_values;
};

struct LB_Context {
    Scope *scope;
    // LB_Block *block;
};

struct LB_Generator {
    Ast_Root *root;
    Source_File *file;
    LLVMModuleRef module;

    LB_Context *context;

    LB_Generator(Source_File *file, Ast_Root *root);
    void generate();
    
    LB_Addr build_addr(Ast_Expr *expr);
    LB_Value build_expr(Ast_Expr *expr);
    LLVMTypeRef build_type(Ast_Type_Info *type_info);

    void build_decl(Ast_Decl *decl);
    LB_Procedure *build_procedure(Ast_Proc *proc);
    void build_procedure_body(LB_Procedure *procedure);
    LB_Struct *build_struct(Ast_Struct *struct_decl);
    
    void build_stmt(Ast_Stmt *stmt);
    void build_block(Ast_Block *block);
};

#endif // LLVM_BACKEND_H
