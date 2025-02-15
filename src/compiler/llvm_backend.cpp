
#include "ast.h"
global Arena *lb_arena;
global Ast_Scope *lb_g_scope;
global LLVMModuleRef lb_g_module;
global LB_Procedure *lb_g_procedure;
global Auto_Array<LB_Procedure*> lb_g_global_procedures;
global Auto_Array<LB_Struct*> lb_g_structs;

#define lb_alloc(T) (T*)lb_backend_alloc(sizeof(T))

internal LB_Procedure *lb_get_procedure(Atom *name) {
    for (int i = 0; i < lb_g_global_procedures.count; i++) {
        LB_Procedure *procedure = lb_g_global_procedures[i];
        if (atoms_match(procedure->name, name)) {
            return procedure;
        }
    }
    return NULL;
}

internal void *lb_backend_alloc(size_t bytes) {
    void *result = (void *)push_array(lb_arena, u8, bytes);
    MemoryZero(result, bytes);
    return result;
}

internal LB_Var *lb_get_named_value(Atom *name) {
    LB_Procedure *proc = lb_g_procedure;
    for (int i = 0; i < proc->named_values.count; i++) {
        LB_Var *var = proc->named_values[i];
        if (atoms_match(var->name, name)) {
            return var;
        }
    }
    return NULL;
}

internal LB_Struct *lb_get_struct(Atom *name) {
    for (int i = 0; i < lb_g_structs.count; i++) {
        LB_Struct *s = lb_g_structs[i];
        if (atoms_match(s->name, name)) {
            return s;
        }
    }
    return NULL;
}
    
internal unsigned lb_get_struct_field_index(Ast_Struct *struct_decl, Atom *name) {
    for (unsigned i = 0; i < struct_decl->fields.count; i++) {
        Ast_Struct_Field *field = struct_decl->fields[i];
        if (atoms_match(field->name, name)) {
            return i;
        }
    }
    Assert(0);
    return 0;
} 

internal LLVMTypeRef lb_build_type(Ast_Type_Info *type_info) {
    LLVMTypeRef result = 0;

    if (type_info->type_flags & TYPE_FLAG_BUILTIN) {
        switch (type_info->builtin_kind) {
        case BUILTIN_TYPE_VOID:
            result = LLVMVoidType();
            break;
        case BUILTIN_TYPE_U8:
            result = LLVMInt8Type();
            break;
        case BUILTIN_TYPE_U16:
            result = LLVMInt16Type();
            break;
        case BUILTIN_TYPE_U32:
            result = LLVMInt32Type();
            break;
        case BUILTIN_TYPE_U64:
            result = LLVMInt64Type();
            break;
        case BUILTIN_TYPE_S8:
            result = LLVMInt8Type();
            break;
        case BUILTIN_TYPE_S16:
            result = LLVMInt16Type();
            break;
        case BUILTIN_TYPE_S32:
            result = LLVMInt32Type();
            break;
        case BUILTIN_TYPE_S64:
            result = LLVMInt64Type();
            break;
        case BUILTIN_TYPE_BOOL:
            result = LLVMInt32Type();
            break;
        case BUILTIN_TYPE_F32:
            result = LLVMFloatType();
            break;
        case BUILTIN_TYPE_F64:
            result = LLVMDoubleType();
            break;
        }
    } else if (type_info->is_struct_type()) {
        LB_Struct *lb_struct = lb_get_struct(type_info->decl->name);
        result = lb_struct->type;
    } else if (type_info->is_array_type()) {
        Ast_Array_Type_Info *array_type_info = static_cast<Ast_Array_Type_Info*>(type_info);
        LLVMTypeRef element_type = lb_build_type(array_type_info->base);
        LLVMTypeRef array_type = LLVMArrayType2(element_type, array_type_info->array_size);
        result = array_type;
    } else if (type_info->is_pointer_type()) {
        LLVMTypeRef element_type = lb_build_type(type_info->base);
        LLVMTypeRef pointer_type = LLVMPointerType(element_type, 0);
        result = pointer_type;
    }

    Assert(result);

    return result;
}

internal LB_Addr lb_build_addr(Ast_Expr *expr) {
    LB_Addr result = {};
    LLVMBuilderRef builder = lb_g_procedure->builder;

    switch (expr->kind) {
    case AST_IDENT:
    {
        Ast_Ident *ident = static_cast<Ast_Ident*>(expr);
        LB_Var *var = lb_get_named_value(ident->name);
        Assert(var);
        result.value = var->alloca;
        break;
    }

    case AST_FIELD:
    {
        Ast_Field *field = static_cast<Ast_Field*>(expr);
        Ast_Field *parent = field->field_parent;
        if (!parent) {
            LB_Addr addr = lb_build_addr(field->elem);
            result.value = addr.value;
        } else {
            LB_Addr addr = lb_build_addr(field->field_parent);
            if (parent->type_info->is_struct_type()) {
                Ast_Ident *name = static_cast<Ast_Ident*>(field->elem);
                Ast_Struct *struct_node = static_cast<Ast_Struct*>(parent->type_info->decl);
                LB_Struct *lb_struct = lb_get_struct(struct_node->name);
                unsigned idx = lb_get_struct_field_index(struct_node, name->name);
                LLVMValueRef field_ptr_value = LLVMBuildStructGEP2(builder, lb_struct->type, addr.value, idx, "fieldaddr_tmp");
                result.value = field_ptr_value;
            }
        }
        break;
    }

    case AST_INDEX:
    {
        Ast_Index *index = static_cast<Ast_Index*>(expr);
        LB_Addr addr = lb_build_addr(index->lhs);
        LB_Value rhs = lb_build_expr(index->rhs);
        LLVMTypeRef type = lb_build_type(index->lhs->type_info);
        LLVMValueRef value = LLVMBuildGEP2(builder, type, addr.value, &rhs.value, 1, "indexaddr");
        result.value = value;
        break;
    }
    }

    return result;
}

internal LB_Value lb_build_expr(Ast_Expr *expr) {
    LB_Value result = {};

    LLVMBuilderRef builder = lb_g_procedure->builder;

    switch (expr->kind) {
    case AST_PAREN:
    {
        Ast_Paren *paren = static_cast<Ast_Paren*>(expr);
        result = lb_build_expr(paren->elem);
        break;
    }

    case AST_LITERAL:
    {
        Ast_Literal *literal = static_cast<Ast_Literal*>(expr);
        LLVMTypeRef type = lb_build_type(literal->type_info);

        if (literal->literal_flags & LITERAL_INT) {
            result.value = LLVMConstInt(type, literal->int_val, (literal->type_info->type_flags & TYPE_FLAG_SIGNED));
        } else if (literal->literal_flags & LITERAL_FLOAT) {
            result.value = LLVMConstReal(type, literal->float_val);
        } else if (literal->literal_flags & LITERAL_STRING) {
        } else if (literal->literal_flags & LITERAL_BOOLEAN) {
            result.value = LLVMConstInt(type, literal->int_val, false);
        }
        Assert(result.value);

        result.type = type; 
        break;
    }

    case AST_COMPOUND_LITERAL:
    {
        Ast_Compound_Literal *literal = static_cast<Ast_Compound_Literal*>(expr);
        break;
    }

    case AST_IDENT:
    {
        Ast_Ident *ident = static_cast<Ast_Ident*>(expr);
        Assert(ident->reference);
        LB_Var *var = lb_get_named_value(ident->name);
        Assert(var);

        result.value = LLVMBuildLoad2(builder, var->type, var->alloca, (char *)var->name->data);
        result.type = lb_build_type(ident->type_info);
        break;
    }

    case AST_CALL:
    {
        Ast_Call *call = static_cast<Ast_Call*>(expr);
        //@Todo Get address of this, instead of this
        if (call->elem->kind == AST_IDENT) {
            Ast_Ident *ident = static_cast<Ast_Ident*>(call->elem);
            LB_Procedure *procedure = lb_get_procedure(ident->name);

            Auto_Array<LLVMValueRef> args;
            if (call->arguments.count) {
                args.reserve(call->arguments.count);
                for (int i = 0; i < call->arguments.count; i++) {
                    Ast_Expr *arg = call->arguments[i];
                    LB_Value arg_value = lb_build_expr(arg);
                    args.push(arg_value.value);
                }
            }

            result.value = LLVMBuildCall2(builder, procedure->type, procedure->value, args.data, (unsigned int)args.count, "calltmp");
            result.type = lb_build_type(call->type_info);
        } else {
            
        }
        break;
    }
    
    case AST_INDEX:
    {
        Ast_Index *index = static_cast<Ast_Index*>(expr);
        LLVMTypeRef array_type = lb_build_type(index->lhs->type_info);
        LLVMTypeRef type = lb_build_type(index->type_info);
        LB_Addr addr = lb_build_addr(index->lhs);
        LB_Value rhs = lb_build_expr(index->rhs);
        LLVMValueRef pointer = LLVMBuildGEP2(builder, array_type, addr.value, &rhs.value, 1, "indexaddr");
        LLVMValueRef value = LLVMBuildLoad2(builder, type, pointer, "indextmp");
        result.value = value;
        break;
    }

    case AST_CAST:
    {
        break;
    }
    
    case AST_ITERATOR:
    {
        break;
    }

    case AST_UNARY:
    {
        Ast_Unary *unary = static_cast<Ast_Unary*>(expr);
        LB_Value elem_value = lb_build_expr(unary->elem);

        if (unary->expr_flags & EXPR_FLAG_OP_CALL) {
            
        } else {
            switch (unary->op.kind) {
            case TOKEN_MINUS:
                result.value = LLVMBuildNeg(builder, elem_value.value, "negtmp");
                break;
            case TOKEN_BANG:
                result.value = LLVMBuildNot(builder, elem_value.value, "nottmp");
                break;
            }
        }

        result.type = lb_build_type(unary->type_info);
        break;
    }

    case AST_ADDRESS:
    {
        Ast_Address *address = static_cast<Ast_Address*>(expr);
        LB_Addr addr = lb_build_addr(address->elem);
        result.value = addr.value;
        break;
    }

    case AST_DEREF:
    {
        Ast_Deref *deref = static_cast<Ast_Deref*>(expr);
        LB_Addr addr = lb_build_addr(deref->elem);
        LLVMTypeRef type = lb_build_type(deref->elem->type_info);
        LLVMValueRef value = LLVMBuildLoad2(builder, type, addr.value, "dereftmp");
        break;
    }

    case AST_BINARY:
    {
        Ast_Binary *binary = static_cast<Ast_Binary*>(expr);


        if (binary->expr_flags & EXPR_FLAG_OP_CALL) {
            
        } else {
            if (binary->is_constant()) {
                LLVMTypeRef type = lb_build_type(binary->type_info);
                if (binary->type_info->is_integral_type()) {
                    result.value = LLVMConstInt(type, binary->eval.int_val, binary->type_info->is_signed());
                } else if (binary->type_info->is_float_type()) {
                    result.value = LLVMConstReal(type, binary->eval.float_val);
                }
            } else {
                LB_Value lhs = lb_build_expr(binary->lhs);
                LB_Value rhs = lb_build_expr(binary->rhs);
                
                switch (binary->op.kind) {
                default:
                    Assert(0);
                    break;

                case TOKEN_PLUS:
                    result.value = LLVMBuildAdd(builder, lhs.value, rhs.value, "addtmp");
                    break;
                case TOKEN_MINUS:
                    result.value = LLVMBuildSub(builder, lhs.value, rhs.value, "subtmp");
                    break;
                case TOKEN_STAR:
                    result.value = LLVMBuildMul(builder, lhs.value, rhs.value, "multmp");
                    break;
                case TOKEN_SLASH:
                    result.value = LLVMBuildSDiv(builder, lhs.value, rhs.value, "sdivtmp");
                    break;
                case TOKEN_MOD:
                    result.value = LLVMBuildSRem(builder, lhs.value, rhs.value, "sremtmp");
                    break;
                case TOKEN_LSHIFT:
                    result.value = LLVMBuildShl(builder, lhs.value, rhs.value, "shltmp");
                    break;
                case TOKEN_RSHIFT:
                    result.value = LLVMBuildLShr(builder, lhs.value, rhs.value, "shrtmp");
                    break;
                case TOKEN_BAR:
                    result.value = LLVMBuildOr(builder, lhs.value, rhs.value, "ortmp");
                    break;
                case TOKEN_AMPER:
                    result.value = LLVMBuildAnd(builder, lhs.value, rhs.value, "andtmp");
                    break;
                case TOKEN_AND:
                    Assert(0); // unsupported
                    break;
                case TOKEN_OR:
                    Assert(0); // unsupported
                    break;
                case TOKEN_EQ2:
                    Assert(0); // unsupported
                    break;
                case TOKEN_LT:
                    Assert(0); // unsupported
                    break;
                case TOKEN_GT:
                    Assert(0); // unsupported
                    break;
                case TOKEN_LTEQ:
                    Assert(0); // unsupported
                    break;
                case TOKEN_GTEQ:
                    Assert(0); // unsupported
                    break;
                }
            }
        }
        result.type = lb_build_type(binary->type_info);
        break;
    }

    case AST_ASSIGNMENT:
    {
        Ast_Assignment *assignment = static_cast<Ast_Assignment*>(expr);
        //@Todo Multiple assignments not supported!
        Assert(assignment->rhs->kind != AST_ASSIGNMENT);

        LB_Addr addr = lb_build_addr(assignment->lhs);

        if (assignment->lhs->type_info->is_struct_type()) {
            Ast_Struct_Type_Info *struct_type = static_cast<Ast_Struct_Type_Info*>(assignment->lhs->type_info);
            LB_Struct *lb_struct = lb_get_struct(struct_type->decl->name);
            
            if (assignment->rhs->kind == AST_COMPOUND_LITERAL) {
                Ast_Compound_Literal *compound = static_cast<Ast_Compound_Literal*>(assignment->rhs);
                for (int i = 0; i < compound->elements.count; i++) {
                    Ast_Expr *elem = compound->elements[i];
                    LB_Value value = lb_build_expr(elem);
                    LLVMValueRef field_addr = LLVMBuildStructGEP2(builder, lb_struct->type, addr.value, (unsigned)i, "fieldaddr_tmp");
                    LLVMBuildStore(builder, value.value, field_addr);
                }
            } else {
                Assert(0);
                //@Todo Get addresses of each field in rhs and store in lhs fields
            }
        } else {
            LB_Addr addr = lb_build_addr(assignment->lhs);
            LB_Value rhs = lb_build_expr(assignment->rhs);

            LLVMBuildStore(builder, rhs.value, addr.value);
        }
        break;
    }

    case AST_FIELD:
    {
        Ast_Field *field = static_cast<Ast_Field*>(expr);

        Ast_Field *parent = field->field_parent;
        Ast_Field *child = field->field_child;

        LB_Addr addr = {};
        if (!parent) {
            addr = lb_build_addr(field->elem);
            result.value = addr.value;
        } else {
            addr = lb_build_addr(parent);
            
            if (parent->type_info->is_struct_type()) {
                Ast_Ident *name = static_cast<Ast_Ident*>(field->elem);
                Ast_Struct *struct_node = static_cast<Ast_Struct*>(parent->type_info->decl);
                LB_Struct *lb_struct = lb_get_struct(struct_node->name);
                unsigned idx = lb_get_struct_field_index(struct_node, name->name);

                LLVMValueRef field_ptr_value = LLVMBuildStructGEP2(builder, lb_struct->type, addr.value, idx, "fieldaddr_tmp");
                addr.value = field_ptr_value;
            }
        }

        if (!child) {
            LLVMTypeRef type = lb_build_type(parent->type_info);
            result.value = LLVMBuildLoad2(builder, type, addr.value, "fieldload");
        }
        break;
    }

    case AST_RANGE:
    {
        break;
    }
    }
    return result;
}

internal void lb_block(LLVMBasicBlockRef basic_block, Ast_Block *block) {
    for (int i = 0; i < block->statements.count; i++) {
        Ast_Stmt *stmt = block->statements[i];
        lb_stmt(basic_block, stmt);
    }
}

internal void lb_stmt(LLVMBasicBlockRef basic_block, Ast_Stmt *stmt) {
    LLVMBuilderRef builder = lb_g_procedure->builder;

    switch (stmt->kind) {
    case AST_EXPR_STMT:
    {
        Ast_Expr_Stmt *expr_stmt = static_cast<Ast_Expr_Stmt*>(stmt);
        Ast_Expr *expr = expr_stmt->expr;
        LB_Value value = lb_build_expr(expr);
        break;
    }

    case AST_DECL_STMT:
    {
        Ast_Decl_Stmt *decl_stmt = static_cast<Ast_Decl_Stmt*>(stmt);
        Ast_Decl *decl = decl_stmt->decl;
        if (decl->kind == AST_VAR) {
            Ast_Var *var_node = static_cast<Ast_Var*>(decl);
            LB_Var *var = lb_alloc(LB_Var);
            var->name = var_node->name;
            var->decl = decl;
            var->type = lb_build_type(var_node->type_info);
            var->alloca = LLVMBuildAlloca(builder, var->type, (char *)var->name->data);
            lb_g_procedure->named_values.push(var);

            if (var_node->init) {
                Ast_Expr *init = var_node->init;
                if (var_node->type_info->is_struct_type()) {
                    LB_Struct *lb_struct = lb_get_struct(var_node->type_info->decl->name);
                    Ast_Compound_Literal *compound = static_cast<Ast_Compound_Literal*>(init);
                    for (int i = 0; i < compound->elements.count; i++) {
                        Ast_Expr *elem = compound->elements[i];
                        LB_Value value = lb_build_expr(elem);
                        LLVMValueRef field_addr = LLVMBuildStructGEP2(builder, lb_struct->type, var->alloca, (unsigned)i, "fieldaddr_tmp");
                        LLVMBuildStore(builder, value.value, field_addr);
                    }
                } else {
                    LB_Value init_value = lb_build_expr(init);
                    LLVMBuildStore(builder, init_value.value, var->alloca);
                }
            } else {
            }
        }
        break;
    }

    case AST_IF:
    {
        break;
    }
    case AST_SWITCH:
    {
        break;
    }
    case AST_WHILE:
    {
        break;
    }
    case AST_FOR:
    {
        break;
    }
    case AST_BLOCK:
    {
        break;
    }

    case AST_RETURN:
    {
        Ast_Return *return_stmt = static_cast<Ast_Return*>(stmt);
        if (return_stmt->expr) {
            LB_Value ret = lb_build_expr(return_stmt->expr);
            LLVMBuildRet(builder, ret.value);
        } else {
            LLVMBuildRetVoid(builder);
        }
        break;
    }

    case AST_GOTO:
    {
        break;
    }
    case AST_DEFER:
    {
        break;
    }
    }
}

internal LB_Procedure *lb_build_procedure(Ast_Proc *proc) {
    Ast_Proc_Type_Info *proc_type = static_cast<Ast_Proc_Type_Info*>(proc->type_info);

    LB_Procedure *lb_proc = lb_alloc(LB_Procedure);
    lb_proc->name = proc->name;
    lb_proc->proc = proc;

    if (proc->parameters.count) lb_proc->parameter_types.reserve(proc_type->parameters.count);

    for (int i = 0; i < proc_type->parameters.count; i++) {
        Ast_Type_Info *type_info = proc_type->parameters[i];
        LLVMTypeRef type = lb_build_type(type_info);
        lb_proc->parameter_types.push(type);
    }
    lb_proc->return_type = lb_build_type(proc_type->return_type);

    lb_proc->type = LLVMFunctionType(lb_proc->return_type, lb_proc->parameter_types.data, (int)lb_proc->parameter_types.count, false);

    lb_proc->value = LLVMAddFunction(lb_g_module, (char *)lb_proc->name->data, lb_proc->type);

    lb_proc->entry = LLVMAppendBasicBlock(lb_proc->value, "entry");

    lb_proc->builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(lb_proc->builder, lb_proc->entry);

    for (int i = 0; i < proc->parameters.count; i++) {
        Ast_Param *param = proc->parameters[i];
        LLVMValueRef param_value = LLVMGetParam(lb_proc->value, i);
        LLVMTypeRef param_type = lb_proc->parameter_types[i];

        LB_Var *var = lb_alloc(LB_Var);
        var->name = param->name;
        var->decl = param;
        var->type = lb_build_type(param->type_info);
        var->alloca = LLVMBuildAlloca(lb_proc->builder, param_type, (char *)var->name->data);
        lb_proc->named_values.push(var);

        LLVMBuildStore(lb_proc->builder, param_value, var->alloca);
    }
    return lb_proc;
}

internal void lb_build_procedure_body(LB_Procedure *procedure) {
    lb_g_procedure = procedure;

    lb_block(procedure->entry, procedure->proc->block);

    if (procedure->proc->type_info == type_void) {
        LLVMBuildRetVoid(procedure->builder);
    }
}

internal LB_Struct *lb_build_struct(Ast_Struct *struct_decl) {
    LB_Struct *lb_struct = lb_alloc(LB_Struct);
    lb_struct->name = struct_decl->name;

    lb_struct->type = LLVMStructCreateNamed(LLVMGetGlobalContext(), (char *)lb_struct->name->data);

    lb_struct->element_types.reserve(struct_decl->fields.count);

    for (int i = 0; i < struct_decl->fields.count; i++) {
        Ast_Struct_Field *field = struct_decl->fields[i];
        LLVMTypeRef field_type = lb_build_type(field->type_info);
        lb_struct->element_types.push(field_type);
    }

    LLVMStructSetBody(lb_struct->type, lb_struct->element_types.data, (unsigned)lb_struct->element_types.count, false);

    return lb_struct;
} 

internal void lb_build_decl(Ast_Decl *decl) {
    switch (decl->kind) {
    case AST_PROC:
    {
        Ast_Proc *proc = static_cast<Ast_Proc*>(decl);
        LB_Procedure *lb_proc = lb_build_procedure(proc);
        lb_g_global_procedures.push(lb_proc);
        break;
    }
    case AST_STRUCT:
    {
        Ast_Struct *struct_decl = static_cast<Ast_Struct*>(decl);
        LB_Struct *lb_struct = lb_build_struct(struct_decl);
        lb_g_structs.push(lb_struct);
        break;
    }
    }
}

internal void lb_backend(Source_File *file, Ast_Root *root) {
    lb_g_module = LLVMModuleCreateWithName("my_module");

    for (int decl_idx = 0; decl_idx < root->declarations.count; decl_idx++) {
        Ast_Decl *decl = root->declarations[decl_idx];
        lb_build_decl(decl);
    }

    for (int proc_idx = 0; proc_idx < lb_g_global_procedures.count; proc_idx++) {
        LB_Procedure *procedure = lb_g_global_procedures[proc_idx];
        lb_build_procedure_body(procedure);
    }

    char *error = NULL;
    LLVMVerifyModule(lb_g_module, LLVMPrintMessageAction, &error);
    LLVMDisposeMessage(error);

    char *gen_file_path = cstring_fmt("%S.bc", path_remove_extension(file->path));
    if (LLVMWriteBitcodeToFile(lb_g_module, gen_file_path) != 0) {
        fprintf(stderr, "error writing bitcode to file %s, skipping\n", gen_file_path);
    }
}
