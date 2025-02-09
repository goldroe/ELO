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

struct LB_Procedure {
    const char *name;
    LLVMValueRef value;
    Auto_Array<LLVMTypeRef> parameters;
    LLVMTypeRef return_type;
    LLVMTypeRef proc_type;
    Ast_Proc *proc;
};

internal void lb_stmt(LLVMBuilderRef builder, LLVMBasicBlockRef basic_block, Ast_Stmt *stmt);

#endif // LLVM_BACKEND_H
