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

struct LB_Struct {
    Atom *name;
    Ast_Struct *decl;

    LLVMTypeRef type;
    Auto_Array<LLVMTypeRef> element_types;
};

struct LB_Var {
    Atom *name;
    Ast_Decl *decl;
    LLVMTypeRef type;
    LLVMValueRef alloca;
};

struct LB_Procedure {
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

internal LB_Addr lb_build_addr(Ast_Expr *expr);
internal LB_Value lb_build_expr(Ast_Expr *expr);
internal void lb_stmt(LLVMBasicBlockRef basic_block, Ast_Stmt *stmt);

#endif // LLVM_BACKEND_H
