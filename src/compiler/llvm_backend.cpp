
global Arena *g_llvm_backend_arena;
global Ast_Scope *g_llvm_scope;
global LLVMModuleRef g_module;
global LB_Procedure *g_current_procedure;

#define llvm_alloc(T) (T*)llvm_backend_alloc(sizeof(T))

internal void *llvm_backend_alloc(size_t bytes) {
    void *result = (void *)push_array(g_llvm_backend_arena, u8, bytes);
    MemoryZero(result, bytes);
    return result;
}

internal LLVMTypeRef lb_type(Ast_Type_Info *type_info) {
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
    }

    return result;
}

internal LLVMValueRef lb_expr(LLVMBuilderRef builder, Ast_Expr *expr) {
    LLVMValueRef result = 0;
    switch (expr->kind) {
    case AST_PAREN:
    {
        Ast_Paren *paren = static_cast<Ast_Paren*>(expr);
        LLVMValueRef value = lb_expr(builder, paren->elem);
        result = value;
        break;
    }

    case AST_LITERAL:
    {
        Ast_Literal *literal = static_cast<Ast_Literal*>(expr);
        LLVMTypeRef type_ref = lb_type(literal->type_info);

        LLVMValueRef value = NULL;
        if (literal->literal_flags & LITERAL_INT) {
            value = LLVMConstInt(type_ref, literal->int_val, (literal->type_info->type_flags & TYPE_FLAG_SIGNED));
        } else if (literal->literal_flags & LITERAL_FLOAT) {
            value = LLVMConstReal(type_ref, literal->float_val);
        } else if (literal->literal_flags & LITERAL_STRING) {
        } else if (literal->literal_flags & LITERAL_BOOLEAN) {
            value = LLVMConstInt(type_ref, literal->int_val, false);
        }
        Assert(value);

        result = value;
        break;
    }

    case AST_COMPOUND_LITERAL:
    {
        break;
    }

    case AST_IDENT:
    {
        Ast_Ident *ident = static_cast<Ast_Ident*>(expr);
        Assert(ident->reference);
        // LLVMValueRef value = 
        break;
    }

    case AST_CALL:
    {
        break;
    }
    case AST_INDEX:
    {
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
        LLVMValueRef elem_value = lb_expr(builder, unary->elem);
        LLVMValueRef value = NULL;

        if (unary->expr_flags & EXPR_FLAG_OP_CALL) {
            
        } else {
            switch (unary->op.kind) {
            case TOKEN_MINUS:
                value = LLVMBuildNeg(builder, elem_value, "neg_u");
                break;
            case TOKEN_BANG:
                value = LLVMBuildNot(builder, elem_value, "not_u");
                break;
            }
        }

        result = value;
        break;
    }

    case AST_ADDRESS:
    {
        break;
    }

    case AST_DEREF:
    {
        break;
    }

    case AST_BINARY:
    {
        Ast_Binary *binary = static_cast<Ast_Binary*>(expr);
        LLVMValueRef value = NULL;

        LLVMValueRef lhs = lb_expr(builder, binary->lhs);
        LLVMValueRef rhs = lb_expr(builder, binary->rhs);

        if (binary->expr_flags & EXPR_FLAG_OP_CALL) {
            
        } else {
            switch (binary->op.kind) {
            default:
                Assert(0);
                break;
            case TOKEN_PLUS:
                value = LLVMBuildAdd(builder, lhs, rhs, "add_b");
                break;
            case TOKEN_MINUS:
                value = LLVMBuildSub(builder, lhs, rhs, "sub_b");
                break;
            case TOKEN_STAR:
                value = LLVMBuildMul(builder, lhs, rhs, "mul_b");
                break;
            case TOKEN_SLASH:
                value = LLVMBuildSDiv(builder, lhs, rhs, "sdiv_b");
                break;
            case TOKEN_MOD:
                value = LLVMBuildSRem(builder, lhs, rhs, "srem_b");
                break;
            case TOKEN_LSHIFT:
                value = LLVMBuildShl(builder, lhs, rhs, "shl_b");
                break;
            case TOKEN_RSHIFT:
                value = LLVMBuildLShr(builder, lhs, rhs, "shr_b");
                break;
            case TOKEN_BAR:
                value = LLVMBuildOr(builder, lhs, rhs, "or_b");
                break;
            case TOKEN_AMPER:
                value = LLVMBuildAnd(builder, lhs, rhs, "and_b");
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

        result = value;
        break;
    }

    case AST_FIELD:
    {
        break;
    }
    case AST_RANGE:
    {
        break;
    }
    }
    return result;
}

internal void lb_block(LLVMBuilderRef builder, LLVMBasicBlockRef basic_block, Ast_Block *block) {
    for (int i = 0; i < block->statements.count; i++) {
        Ast_Stmt *stmt = block->statements[i];
        lb_stmt(builder, basic_block, stmt);
    }
}

internal void lb_stmt(LLVMBuilderRef builder, LLVMBasicBlockRef basic_block, Ast_Stmt *stmt) {
    switch (stmt->kind) {
    case AST_EXPR_STMT:
    {
        Ast_Expr_Stmt *expr_stmt = static_cast<Ast_Expr_Stmt*>(stmt);
        Ast_Expr *expr = expr_stmt->expr;
        LLVMValueRef value = lb_expr(builder, expr);
        break;
    }
    case AST_DECL_STMT:
    {
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
            LLVMValueRef ret_value = lb_expr(builder, return_stmt->expr);
            LLVMBuildRet(builder, ret_value);
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

internal LB_Procedure *lb_procedure(Ast_Proc *proc) {
    Ast_Proc_Type_Info *proc_type = static_cast<Ast_Proc_Type_Info*>(proc->type_info);

    LB_Procedure *lb_proc = llvm_alloc(LB_Procedure);
    lb_proc->name = (const char *)proc->name->data;
    lb_proc->proc = proc;

    if (proc->parameters.count) lb_proc->parameters.reserve(proc_type->parameters.count);

    for (int i = 0; i < proc_type->parameters.count; i++) {
        Ast_Type_Info *type_info = proc_type->parameters[i];
        LLVMTypeRef type = lb_type(type_info);
        lb_proc->parameters.push(type);
    }
    lb_proc->return_type = lb_type(proc_type->return_type);

    lb_proc->proc_type = LLVMFunctionType(lb_proc->return_type, lb_proc->parameters.data, (int)lb_proc->parameters.count, false);

    lb_proc->value = LLVMAddFunction(g_module, lb_proc->name, lb_proc->proc_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(lb_proc->value, "entry");

    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);

    lb_block(builder, entry, proc->block);

    if (proc->type_info == type_void) {
        LLVMBuildRetVoid(builder);
    }

    return lb_proc;
}

internal void llvm_backend(Source_File *file, Ast_Root *root) {
    g_module = LLVMModuleCreateWithName("my_module");

    for (int decl_idx = 0; decl_idx < root->declarations.count; decl_idx++) {
        Ast_Decl *decl = root->declarations[decl_idx];
        if (decl->kind == AST_PROC) {
            Ast_Proc *proc = static_cast<Ast_Proc*>(decl);
            LB_Procedure *lb_proc = lb_procedure(proc);
        }
    }

    char *error = NULL;
    LLVMVerifyModule(g_module, LLVMPrintMessageAction, &error);
    LLVMDisposeMessage(error);

    char *gen_file_path = cstring_fmt("%S.bc", path_remove_extension(file->path));
    if (LLVMWriteBitcodeToFile(g_module, gen_file_path) != 0) {
        fprintf(stderr, "error writing bitcode to file %s, skipping\n", gen_file_path);
    }
}
