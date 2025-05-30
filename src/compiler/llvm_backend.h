#ifndef LLVM_BACKEND_H
#define LLVM_BACKEND_H

#pragma warning(push)
#pragma warning(disable : 4127)
#pragma warning(disable : 4244)
#pragma warning(disable : 4245)
#pragma warning(disable : 4267)
#pragma warning(disable : 4310)
#pragma warning(disable : 4324)
#pragma warning(disable : 4457)
#pragma warning(disable : 4458)
#pragma warning(disable : 4624)
#pragma warning(disable : 4996)
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#pragma warning(pop)

struct LLVM_Decl;

struct LLVM_Value {
    llvm::Value *value;
    llvm::Type *type;
};

struct LLVM_Addr {
    llvm::Value *value;
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
    llvm::Type* type;
    Auto_Array<llvm::Type*> element_types;
    Ast_Struct *decl;
};

struct LLVM_Var : LLVM_Decl {
    Atom *name;
    Ast_Decl *decl;
    llvm::Type* type;
    llvm::AllocaInst *alloca;
};

struct LLVM_Procedure : LLVM_Decl {
    Atom *name;
    Ast_Proc *proc;

    llvm::Function *fn;
    llvm::FunctionType* type;
    llvm::Type* return_type;
    Auto_Array<llvm::Type*> parameter_types;

    llvm::IRBuilder<> *builder;
    llvm::BasicBlock *entry;

    Auto_Array<LLVM_Var*> named_values;

    llvm::AllocaInst *return_value;
    llvm::BasicBlock *exit_block;
};

struct LLVM_Backend {
    Ast_Root *ast_root;
    Source_File *file;

    llvm::LLVMContext *Ctx;
    llvm::Module *Module;

    llvm::IRBuilder<> *builder;

    Auto_Array<LLVM_Procedure*> global_procedures;
    Auto_Array<LLVM_Struct*> global_structs;

    LLVM_Procedure *current_proc = nullptr;
    llvm::BasicBlock *current_block = nullptr;

    llvm::StructType *builtin_string_type;

    LLVM_Backend(Source_File *file, Ast_Root *root) : ast_root(root), file(file) {}
    
    void gen();
    
    llvm::Type* get_type(Ast_Type_Info *type_info);
    LLVM_Addr gen_addr(Ast_Expr *expr);
    llvm::Value* gen_condition(Ast_Expr *expr);

    LLVM_Value gen_expr(Ast_Expr *expr);
    LLVM_Value gen_binary_op(Ast_Binary *binop);

    void gen_decl(Ast_Decl *decl);
    void gen_param(Ast_Param *param);
    void gen_var(Ast_Var *var_node);

    void set_procedure(LLVM_Procedure *procedure);
    LLVM_Procedure *gen_procedure(Ast_Proc *proc);
    void gen_procedure_body(LLVM_Procedure *procedure);
    LLVM_Struct *gen_struct(Ast_Struct *struct_decl);
    
    void gen_stmt(Ast_Stmt *stmt);
    void gen_if(Ast_If *if_stmt);
    void gen_while(Ast_While *while_stmt);
    void gen_for(Ast_For *for_stmt);
    void gen_block(Ast_Block *block);
    void gen_statement_list(Auto_Array<Ast_Stmt*> statement_list);

    llvm::BasicBlock *llvm_block_new(const char *s = "");
    void llvm_store(llvm::Value *value, llvm::Value *address);

    LLVM_Procedure *lookup_proc(Atom *name);
    LLVM_Struct *LLVM_Backend::lookup_struct(Atom *name);

    void get_lazy_expressions(Ast_Binary *root, Token_Kind op, Auto_Array<Ast_Expr*> *expr_list);
    void lazy_eval(Ast_Binary *root, llvm::PHINode *phi_node, llvm::BasicBlock *exit_block);

    bool emit_block_check_branch();
    void emit_block(llvm::BasicBlock *block);
    void emit_jump(llvm::BasicBlock *target);

    void gen_fallthrough(Ast_Fallthrough *fallthrough);
    void gen_break(Ast_Break *break_stmt);
    void gen_return(Ast_Return *return_stmt);
    void gen_continue(Ast_Continue *continue_stmt);

    void gen_branch(llvm::BasicBlock *target_block);
    void gen_branch_condition(llvm::Value *condition, llvm::BasicBlock *true_block, llvm::BasicBlock *false_block);

    void gen_ifcase(Ast_Ifcase *ifcase);
    void gen_ifcase_switch(Ast_Ifcase *ifcase);
    void gen_ifcase_if_else(Ast_Ifcase *ifcase);
};

#endif // LLVM_BACKEND_H
