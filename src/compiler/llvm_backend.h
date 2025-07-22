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
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#pragma warning(pop)

struct LLVM_Value {
    llvm::Value *value;
    llvm::Type *type;
};

struct LLVM_Addr {
    llvm::Value *value;
};

struct BE_Struct {
    Atom *name;
    llvm::Type* type;
    Array<llvm::Type*> element_types;
    Type_Struct *st = nullptr;
};

struct BE_Var {
    Atom *name;
    Decl *decl;
    llvm::Type* type;

    //@Note AllocaInt, GlobalVariable
    llvm::Value *storage;
};

struct BE_Proc {
    Atom *name;
    Ast_Proc_Lit *proc_lit;

    llvm::Function *fn;
    llvm::FunctionType* type;
    llvm::Type *results;
    Array<llvm::Type*> params;

    llvm::IRBuilder<> *builder;
    llvm::BasicBlock *entry;

    Array<BE_Var*> named_values;

    llvm::AllocaInst *return_value;
    llvm::BasicBlock *exit_block;
};

struct LLVM_Backend {
    Ast_Root *ast_root;
    Source_File *file;

    llvm::LLVMContext *Ctx;
    llvm::Module *Module;

    llvm::IRBuilder<> *Builder;

    BE_Proc *current_proc = nullptr;
    llvm::BasicBlock *current_block = nullptr;

    llvm::StructType *builtin_string_type;

    LLVM_Backend(Source_File *file, Ast_Root *root) : ast_root(root), file(file) {}
    
    void gen();
    
    llvm::Type* get_type(Type *type_info);
    LLVM_Addr gen_addr(Ast *expr);
    llvm::Value* gen_condition(Ast *expr);


    llvm::Value *gen_logical_not(llvm::Value *value);

    LLVM_Value gen_expr(Ast *expr);
    LLVM_Value gen_binary_op(Ast_Binary *binop);

    void gen_param(Ast_Param *param);
    // void gen_var(Ast_Var *var_node);

    void set_procedure(BE_Proc *procedure);
    void gen_procedure_body(Decl *decl);
    
    void gen_assignment_stmt(Ast_Assignment *assignment);
    void gen_stmt(Ast *stmt);
    void gen_if(Ast_If *if_stmt);
    void gen_while(Ast_While *while_stmt);
    void gen_for_stmt(Ast_For *for_stmt);
    void gen_range_stmt(Ast_Range_Stmt *range_stmt);
    void gen_block(Ast_Block *block);

    void gen_statement_list(Array<Ast*> statement_list);

    llvm::BasicBlock *llvm_block_new(const char *s = "");


    llvm::Value *LLVM_Backend::llvm_pointer_offset(llvm::Type *type, llvm::Value *ptr, llvm::Value *value);
    llvm::Value *LLVM_Backend::llvm_pointer_offset(llvm::Type *type, llvm::Value *ptr, unsigned index);
    llvm::Value *llvm_struct_gep(llvm::Type *type, llvm::Value *ptr, unsigned index);
    void llvm_store(llvm::Value *value, llvm::Value *address);

    BE_Proc *lookup_proc(Atom *name);
    BE_Struct *LLVM_Backend::lookup_struct(Atom *name);

    void get_lazy_expressions(Ast_Binary *root, OP op, Array<Ast*> *expr_list);
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

    LLVM_Value LLVM_Backend::gen_field_select(llvm::Value *base, Type *type, Atom *name, int index);

    LLVM_Value LLVM_Backend::gen_literal(Ast_Literal *literal);

    void LLVM_Backend::gen_assignment(Ast_Assignment *assignment);


    llvm::Value *LLVM_Backend::gen_constant_value(Constant_Value value);

    void gen_type_struct(Type_Struct *ts);

    void gen_decl(Decl *decl);
    void LLVM_Backend::gen_decl_variable(Decl *decl);
    void LLVM_Backend::gen_decl_type(Decl *decl);


    LLVM_Value LLVM_Backend::gen_constant_value(Constant_Value value, Type *type);

    void LLVM_Backend::init_value_decl(Ast_Value_Decl *vd, bool is_global);
    void LLVM_Backend::gen_value_decl(Ast_Value_Decl *vd, bool is_global);

    void LLVM_Backend::gen_decl_procedure(Decl *decl);
};

#endif // LLVM_BACKEND_H
