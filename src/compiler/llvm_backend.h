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

struct LLVM_Decl;

struct LLVM_Value {
    LLVMValueRef value;
    LLVMTypeRef type;
};

struct LLVM_Addr {
    LLVMValueRef value;
};

enum LLVM_Decl_Kind {
    LLVM_DECL_VAR,
    LLVM_DECL_PROC,
    LLVM_DECL_STRUCT
};

struct LLVM_Decl {
    LLVM_Decl_Kind kind;
};

struct LLVM_Struct : LLVM_Decl {
    Atom *name;
    LLVMTypeRef type;
    Auto_Array<LLVMTypeRef> element_types;
    Ast_Struct *decl;
};

struct LLVM_Var : LLVM_Decl {
    Atom *name;
    Ast_Decl *decl;
    LLVMTypeRef type;
    LLVMValueRef alloca;
};

struct LLVM_Procedure : LLVM_Decl {
    Atom *name;
    Ast_Proc *proc;

    LLVMValueRef value;
    LLVMTypeRef type;
    Auto_Array<LLVMTypeRef> parameter_types;
    LLVMTypeRef return_type;

    LLVMBuilderRef builder;
    LLVMBasicBlockRef entry;

    Auto_Array<LLVM_Var*> named_values;
};

struct LLVM_Generator {
    Ast_Root *root;
    Source_File *file;

    LLVMModuleRef module;
    LLVMContextRef context;
    LLVMBuilderRef builder;

    Auto_Array<LLVM_Procedure*> global_procedures;
    Auto_Array<LLVM_Struct*> global_structs;

    LLVM_Procedure *current_proc;
    LLVMBasicBlockRef current_block;

    LLVM_Generator(Source_File *file, Ast_Root *root);
    void generate();
    
    LLVM_Addr build_addr(Ast_Expr *expr);
    LLVM_Value build_expr(Ast_Expr *expr);
    LLVMTypeRef build_type(Ast_Type_Info *type_info);

    void build_decl(Ast_Decl *decl);
    LLVM_Procedure *build_procedure(Ast_Proc *proc);
    void build_procedure_body(LLVM_Procedure *procedure);
    LLVM_Struct *build_struct(Ast_Struct *struct_decl);
    
    void build_stmt(Ast_Stmt *stmt);
    void build_block(Ast_Block *block);

    LLVMValueRef build_condition(Ast_Expr *expr);

    LLVMBasicBlockRef llvm_block_new(const char *s);
    void llvm_emit_block(LLVMBasicBlockRef block);

    LLVM_Procedure *lookup_proc(Atom *name);
    LLVM_Struct *LLVM_Generator::lookup_struct(Atom *name);
};

#endif // LLVM_BACKEND_H
