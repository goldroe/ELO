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

// #include <llvm-c/Transforms/AggressiveInstCombine.h>
// #include <llvm-c/Transforms/InstCombine.h>
// #include <llvm-c/Transforms/IPO.h>
// #include <llvm-c/Transforms/PassManagerBuilder.h>
// #include <llvm-c/Transforms/Scalar.h>
// #include <llvm-c/Transforms/Utils.h>
// #include <llvm-c/Transforms/Vectorize.h>

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

internal void lb_stmt(LLVMBasicBlockRef basic_block, Ast_Stmt *stmt);

#endif // LLVM_BACKEND_H
