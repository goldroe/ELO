global Arena *llvm_arena;

void LLVM_Backend::get_lazy_expressions(Ast_Binary *root, OP op, Auto_Array<Ast*> *expr_list) {
    if (is_binop(root->lhs, op)) {
        Ast_Binary *child = static_cast<Ast_Binary*>(root->lhs);
        get_lazy_expressions(child, op, expr_list);
        expr_list->push(root);
    } else {
        expr_list->push(root->lhs);
        expr_list->push(root->rhs);
    }
}

void LLVM_Backend::lazy_eval(Ast_Binary *root, llvm::PHINode *phi_node, llvm::BasicBlock *exit_block) {
    Auto_Array<Ast*> lazy_hierarchy;
    get_lazy_expressions(root, root->op, &lazy_hierarchy);

    llvm::Value *lazy_constant;
    if (root->op == OP_AND) lazy_constant = llvm::ConstantInt::getFalse(llvm::Type::getInt1Ty(*Ctx));
    else lazy_constant = llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(*Ctx));

    for (Ast *expr : lazy_hierarchy) {
        bool is_tail = expr == lazy_hierarchy.back();

        llvm::Value *value = NULL;
        if (is_binop(expr, root->op)) {
            Ast_Binary *binary = static_cast<Ast_Binary*>(expr);
            value = gen_condition(binary->rhs);
        } else {
            value = gen_condition(expr);
        }

        llvm::BasicBlock *phi_block = current_block;
        if (is_tail) {
            gen_branch(exit_block);
            phi_node->addIncoming(value, phi_block);
            emit_block(exit_block);
        } else {
            llvm::BasicBlock *next_block = llvm_block_new();
            if (root->op == OP_OR) {
                gen_branch_condition(value, exit_block, next_block);
            } else {
                gen_branch_condition(value, next_block, exit_block);
            }
            phi_node->addIncoming(lazy_constant, phi_block);
            emit_block(next_block);
        }
    }
}

LLVM_Value LLVM_Backend::gen_binary_op(Ast_Binary *binop) {
    LLVM_Value result = {};
    result.type = get_type(binop->type);

    if (binop->op == OP_AND || binop->op == OP_OR) {
        llvm::BasicBlock *lazy_exit = llvm_block_new("lazy.exit");
        llvm::PHINode *phi_node = llvm::PHINode::Create(llvm::Type::getInt1Ty(*Ctx), 0);
        lazy_eval(binop, phi_node, lazy_exit);
        llvm::Value *phi_value = Builder->Insert(phi_node);
        result.value = phi_value;
    } else {
        LLVM_Value lhs = gen_expr(binop->lhs);
        LLVM_Value rhs = gen_expr(binop->rhs);

        bool is_float = is_float_type(binop->type);
        bool is_signed = is_signed_type(binop->type);

        switch (binop->op) {
        default:
            Assert(0);
            break;

        case OP_ADD:
            if (is_integral_type(binop->type)) {
                result.value = Builder->CreateAdd(lhs.value, rhs.value);
            } else if (is_float_type(binop->type)) {
                result.value = Builder->CreateFAdd(lhs.value, rhs.value);
            } else if (is_pointer_type(binop->type)) {
                llvm::Type *type = get_type(binop->type->base);
                if (is_pointer_type(binop->lhs->type)) {
                    result.value = Builder->CreateGEP(type, lhs.value, rhs.value);
                } else {
                    result.value = Builder->CreateGEP(type, rhs.value, lhs.value);
                }
            }
            break;
        case OP_SUB:
            result.value = Builder->CreateSub(lhs.value, rhs.value);
            break;
        case OP_MUL:
            if (is_float) result.value = Builder->CreateFMul(lhs.value, rhs.value);
            else          result.value = Builder->CreateMul(lhs.value, rhs.value);
            break;
        case OP_DIV:
            result.value = Builder->CreateSDiv(lhs.value, rhs.value);
            break;
        case OP_MOD:
            result.value = Builder->CreateSRem(lhs.value, rhs.value);
            break;
        case OP_LSH:
            result.value = Builder->CreateShl(lhs.value, rhs.value);
            break;
        case OP_RSH:
            result.value = Builder->CreateLShr(lhs.value, rhs.value);
            break;
        case OP_BIT_OR:
            result.value = Builder->CreateOr(lhs.value, rhs.value);
            break;
        case OP_BIT_AND:
            result.value = Builder->CreateAnd(lhs.value, rhs.value);
            break;
        case OP_NEQ:
            result.value = Builder->CreateICmp(llvm::CmpInst::ICMP_NE, lhs.value, rhs.value);
            break;
        case OP_EQ:
            result.value = Builder->CreateICmp(llvm::CmpInst::ICMP_EQ, lhs.value, rhs.value);
            break;
        case OP_LT:
            result.value = Builder->CreateICmp(llvm::CmpInst::ICMP_SLT, lhs.value, rhs.value);
            break;
        case OP_GT:
            result.value = Builder->CreateICmp(llvm::CmpInst::ICMP_SGT, lhs.value, rhs.value);
            break;
        case OP_LTEQ:
            result.value = Builder->CreateICmp(llvm::CmpInst::ICMP_SLE, lhs.value, rhs.value);
            break;
        case OP_GTEQ:
            result.value = Builder->CreateICmp(llvm::CmpInst::ICMP_SGE, lhs.value, rhs.value);
            break;
        }
    }
    return result;
}

#define llvm_alloc(T) (T*)llvm_backend_alloc(sizeof(T), alignof(T))
internal void *llvm_backend_alloc(u64 size, int alignment) {
    void *result = (void *)arena_alloc(llvm_arena, size, alignment);
    MemoryZero(result, size);
    return result;
}

llvm::BasicBlock *LLVM_Backend::llvm_block_new(const char *s) {
    llvm::BasicBlock *basic_block = llvm::BasicBlock::Create(*Ctx, s);
    return basic_block;
}

internal inline llvm::Constant *llvm_const_int(llvm::Type *type, u64 value) {
    return llvm::ConstantInt::get(type, 0);
}

internal inline llvm::Constant *llvm_zero(llvm::Type *type) {
    return llvm_const_int(type, 0);
}

llvm::Value *LLVM_Backend::gen_logical_not(llvm::Value *value) {
    llvm::Type *type = value->getType();
    auto size_in_bits = type->getPrimitiveSizeInBits();
    if (type->isFloatingPointTy()) {
        return Builder->CreateFCmpOEQ(value, llvm::ConstantFP::get(Builder->getFloatTy(), 0.0));
    } else if (type->isSingleValueType()) {
        return Builder->CreateICmpEQ(value, llvm::ConstantInt::get(Builder->getIntNTy((unsigned)size_in_bits), 0, false));
    }
    return value;
}

void LLVM_Backend::emit_block(llvm::BasicBlock *block) {
    Assert(current_block == NULL);
    llvm::Function *fn = current_proc->fn;
    fn->insert(fn->end(), block);
    Builder->SetInsertPoint(block);
    current_block = block;
}

bool LLVM_Backend::emit_block_check_branch() {
    if (!current_block) {
        return false;
    }

    // remove blocks that do not have any predecessors
    if (current_block != current_proc->entry &&
        current_block->use_begin() == current_block->use_end()) {
        current_block->eraseFromParent();
        current_block = nullptr;
        return false;
    }
    return true;
}

void LLVM_Backend::gen_branch_condition(llvm::Value *condition, llvm::BasicBlock *true_block, llvm::BasicBlock *false_block) {
    if (!emit_block_check_branch()) {
        return;
    }
    current_block = nullptr;
    Builder->CreateCondBr(condition, true_block, false_block);
}

void LLVM_Backend::gen_branch(llvm::BasicBlock *target_block) {
    if (!emit_block_check_branch()) {
        return;
    }
    current_block = nullptr;
    Builder->CreateBr(target_block);
}


//@Todo Bring these functions back into the fold
#if 0
void LLVM_Backend::gen_for(Ast_For *for_stmt) {
    // for init; cond; it {
    // ...
    // }
    // gen init
    // label loop_head:
    // gen cond
    // ifcmp neq, cond, 0
    // br loop_body, loop_tail
    // label loop_body:
    // ...
    // gen it
    // br loop_head
    // label loop_tail:
    // for it in iterator {
    // ...
    // }

    Assert(for_stmt->iterator->kind == AST_RANGE);
    Assert(for_stmt->var->backend_var);

    BE_Var *var = for_stmt->var->backend_var;
    Ast_Range *range = static_cast<Ast_Range*>(for_stmt->iterator);
    llvm::Type *it_type = get_type(range->lhs->type);
    LLVM_Value range_max = gen_expr(range->rhs);

    gen_var(for_stmt->var);

    llvm::BasicBlock *body = llvm_block_new();
    for_stmt->entry_block = llvm_block_new();
    for_stmt->exit_block = llvm_block_new();
    for_stmt->retry_block = llvm_block_new();

    Builder->CreateBr((llvm::BasicBlock *)for_stmt->retry_block);

    current_block = nullptr;

    emit_block((llvm::BasicBlock *)for_stmt->entry_block);
    llvm::Value *it_value = Builder->CreateLoad(var->type, var->alloca);
    llvm::Value *iterate = Builder->CreateAdd(it_value, llvm::ConstantInt::get(it_type, 1, false));
    Builder->CreateStore(iterate, var->alloca);
    Builder->CreateBr((llvm::BasicBlock *)for_stmt->retry_block);

    current_block = nullptr;

    emit_block((llvm::BasicBlock *)for_stmt->retry_block);
    it_value = Builder->CreateLoad(var->type, var->alloca);
    llvm::Value *condition = Builder->CreateICmp(llvm::CmpInst::ICMP_SLT, it_value, range_max.value);
    gen_branch_condition(condition, body, (llvm::BasicBlock *)for_stmt->exit_block);

    emit_block(body);
    gen_block(for_stmt->block);

    gen_branch((llvm::BasicBlock *)for_stmt->entry_block);

    emit_block((llvm::BasicBlock *)for_stmt->exit_block);
}

void LLVM_Backend::gen_var(Ast_Var *var_node) {
    //@Note Don't generate backend value if variable is a constant expression
    if (var_node->decl_flags & DECL_FLAG_CONST) return;

    if (var_node->decl_flags & DECL_FLAG_GLOBAL) {
        BE_Var *var = llvm_alloc(BE_Var);
        var->name = var_node->name;
        var->decl = var_node;
        var->type = get_type(var_node->type);

        var->alloca = nullptr;
        llvm::GlobalVariable *global_variable = new llvm::GlobalVariable(*Module, var->type, false, llvm::GlobalValue::ExternalLinkage, nullptr, (char *)var->name->data);

        llvm::Constant *initializer = nullptr;
        if (var_node->init) {
            LLVM_Value const_value = gen_expr(var_node->init);
            initializer = static_cast<llvm::Constant*>(const_value.value);
        } else {
            initializer = llvm::Constant::getNullValue(var->type);
        }
         global_variable->setInitializer(initializer);

        var->global_variable = global_variable;
        var_node->backend_var = var;
    } else {
        // var->name = var_node->name;
        // var->decl = var_node;
        // var->type = get_type(var_node->type);
        // var->alloca = Builder->CreateAlloca(var->type, 0, nullptr, llvm::Twine(var->name->data));
        // current_proc->named_values.push(var);
        BE_Var *var = var_node->backend_var;
        Assert(var);

        if (!var_node->init) {
            llvm::Constant *null_value = llvm::Constant::getNullValue(var->type);
            Builder->CreateStore(null_value, var->alloca);
        }

        if (var_node->init) {
            Ast *init = var_node->init;
            if (init->kind == AST_COMPOUND_LITERAL) {
                Ast_Compound_Literal *literal = static_cast<Ast_Compound_Literal*>(init);
                if (var_node->type->is_struct_type()) {
                    BE_Struct *be_struct = ((Ast_Struct*)var_node->type->decl)->backend_struct;
                    for (int i = 0; i < literal->elements.count; i++) {
                        Ast *elem = literal->elements[i];
                        LLVM_Value value = gen_expr(elem);
                        llvm::Value* field_addr = Builder->CreateStructGEP(be_struct->type, var->alloca, (unsigned)i);
                        Builder->CreateStore(value.value, field_addr);
                    }
                } else if (var_node->type->is_array_type()) {
                    llvm::ArrayType* array_type = static_cast<llvm::ArrayType*>(get_type(var_node->type));
                    llvm::Type* element_type = array_type->getElementType();

                    //@Todo Have to infer if array is filled with constants to just use llvm::ConstantDataArray
                    int i = 0;
                    for (Ast *elem : literal->elements) {
                        LLVM_Value value = gen_expr(elem);

                        llvm::Value *idx = llvm::ConstantInt::get(llvm::IntegerType::get(*Ctx, 32), (u64)i);
                        llvm::ArrayRef<llvm::Value*> indices = {
                            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Ctx), 0),
                            idx
                        };
                        llvm::Value *ptr = Builder->CreateGEP(array_type, var->alloca, indices);
                        Builder->CreateStore(value.value, ptr);
                        i++;
                    }
                }
            } else if (init->kind == AST_RANGE) {
                Ast_Range *range = static_cast<Ast_Range*>(init);
                LLVM_Value init_value = gen_expr(range->lhs);
                Builder->CreateStore(init_value.value, var->alloca);
            } else if (init->type->is_array_type()) {
                LLVM_Value value = gen_expr(init);
                llvm::ArrayRef<llvm::Value*> indices = {
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Ctx), 0),
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Ctx), 0)
                };
                llvm::Value *ptr = Builder->CreateGEP(value.type, value.value, indices);
                Builder->CreateStore(ptr, var->alloca);
            } else {
                LLVM_Value init_value = gen_expr(init);
                Builder->CreateStore(init_value.value, var->alloca);
            }
        }

    }
}

#endif

void LLVM_Backend::gen_assignment_stmt(Ast_Assignment *assignment) {
    if (assignment->op == OP_ASSIGN) {
        for (int v = 0, l = 0; v < assignment->rhs.count; v++) {
            Ast *value_expr = assignment->rhs[v];
            LLVM_Value value = gen_expr(value_expr);
            int value_count = get_value_count(value_expr->type);
            if (value_count == 1) {
                Ast *lhs = assignment->lhs[l];
                LLVM_Addr addr = gen_addr(lhs);
                llvm_store(value.value, addr.value);
                l++;
            } else {
                for (int i = 0; i < value_count; i++, l++) {
                    Ast *lhs = assignment->lhs[l];
                    LLVM_Addr addr = gen_addr(lhs);
                    llvm::Value *ptr = llvm_struct_gep(value.type, value.value, i);
                    llvm_store(ptr, addr.value);
                }
            }
        }
    } else {
        Ast *lhs = assignment->lhs[0];
        Ast *rhs = assignment->rhs[0];

        LLVM_Addr addr = gen_addr(lhs);
        LLVM_Value lhs_val = gen_expr(lhs);
        LLVM_Value rhs_val = gen_expr(rhs);

        llvm::Value *value = nullptr;

        switch (assignment->op) {
        case OP_ADD_ASSIGN:
            value = Builder->CreateAdd(lhs_val.value, rhs_val.value);
            break;
        case OP_SUB_ASSIGN:
            value = Builder->CreateSub(lhs_val.value, rhs_val.value);
            break;
        case OP_MUL_ASSIGN:
            value = Builder->CreateMul(lhs_val.value, rhs_val.value);
            break;
        case OP_DIV_ASSIGN:
            value = Builder->CreateSDiv(lhs_val.value, rhs_val.value);
            break;
        case OP_MOD_ASSIGN:
            value = Builder->CreateSRem(lhs_val.value, rhs_val.value);
            break;
        case OP_AND_ASSIGN:
            value = Builder->CreateAnd(lhs_val.value, rhs_val.value);
            break;
        case OP_OR_ASSIGN:
            value = Builder->CreateOr(lhs_val.value, rhs_val.value);
            break;
        case OP_XOR_ASSIGN:
            value = Builder->CreateXor(lhs_val.value, rhs_val.value);
            break;
        case OP_LSH_ASSIGN:
            value = Builder->CreateShl(lhs_val.value, rhs_val.value);
            break;
        case OP_RSH_ASSIGN:
            value = Builder->CreateLShr(lhs_val.value, rhs_val.value);
            break;
        }

        llvm_store(value, addr.value);
    }
}

LLVM_Value LLVM_Backend::gen_constant_value(Constant_Value constant_value, Type *type) {
    LLVM_Value value = {};
    llvm::Type *ty = get_type(type);
    value.type = ty;
    switch (constant_value.kind) {
    case CONSTANT_VALUE_INTEGER: {
        value.value = llvm::ConstantInt::get(ty, u64_from_bigint(constant_value.value_integer), is_signed_type(type));
        break;
    }
    case CONSTANT_VALUE_FLOAT: {
        value.value = llvm::ConstantFP::get(ty, constant_value.value_float);
        break;
    }
    case CONSTANT_VALUE_STRING: {
        llvm::Constant *constant_string = llvm::ConstantDataArray::getString(*Ctx, (char *)constant_value.value_string.data, false);
        llvm::Value *var = Builder->CreateGlobalString((char *)constant_value.value_string.data);
        value.value = var;
        value.type = llvm::ArrayType::get(llvm::Type::getInt8Ty(*Ctx), constant_value.value_string.count);
        break;
    }
    }
    return value;
}

LLVM_Value LLVM_Backend::gen_expr(Ast *expr) {
    if (!expr) return {};

    if (expr->mode == ADDRESSING_CONSTANT) {
        return gen_constant_value(expr->value, expr->type);
    }

    switch (expr->kind) {
    default:
        Assert(0);
        break;

    case AST_PAREN: {
        Ast_Paren *paren = static_cast<Ast_Paren*>(expr);
        return gen_expr(paren->elem);
    }

    case AST_COMPOUND_LITERAL: {
        Ast_Compound_Literal *literal = static_cast<Ast_Compound_Literal*>(expr);
        break;
    }

    case AST_IDENT: {
        LLVM_Value value;
        Ast_Ident *ident = static_cast<Ast_Ident*>(expr);
        Assert(ident->ref);
        Decl *decl = ident->ref;
        switch (decl->kind) {
        default: Assert(0); break;

        case DECL_CONSTANT:
            //@Todo should be caught earlier?
            Assert(0);
            break;

        case DECL_VARIABLE: {
            BE_Var *be_var = decl->backend_var;
            bool is_global = false;
            if (is_global) {
                if (is_array_type(ident->type)) {
                    value.value = be_var->global_variable;
                } else {
                    llvm::LoadInst *load = Builder->CreateLoad(be_var->type, be_var->global_variable);
                    value.value = load;
                }
                value.type = be_var->type;
            } else {
                Assert(be_var);
                if (is_array_type(ident->type)) {
                    value.value = be_var->alloca;
                } else {
                    llvm::LoadInst *load = Builder->CreateLoad(be_var->type, be_var->alloca);
                    value.value = load;
                }
                value.type = be_var->type;
            }
            value.value->setName((char *)ident->name->data);
            return value;
        }

        case DECL_PROCEDURE: {
            BE_Proc *backend_proc = decl->proc_lit->backend_proc;
            value.value = backend_proc->fn;
            value.type = backend_proc->type;
            return value;
        }
        }
    }

    case AST_CALL: {
        Ast_Call *call = static_cast<Ast_Call*>(expr);
        LLVM_Value value = {};

        llvm::Value *callee = nullptr;
        llvm::FunctionType *fn_ty = nullptr;
        llvm::Type *return_ty = nullptr;

        //@Todo Get address of this, instead of this
        // LLVM_Addr addr = gen_addr(call->elem);
        if (call->elem->kind == AST_IDENT) {
            Ast_Ident *ident = (Ast_Ident *)call->elem;
            Decl *decl = ident->ref;
            switch (decl->kind) {
            case DECL_PROCEDURE: {
                BE_Proc *procedure = decl->proc_lit->backend_proc;
                fn_ty = procedure->type;
                callee = procedure->fn;
                return_ty = procedure->results;
                break;
            }

            case DECL_VARIABLE: {
                Type_Proc *tp = (Type_Proc *)ident->type;
                Auto_Array<llvm::Type*> params;
                for (Type *param : tp->params->types) {
                    llvm::Type *param_type = get_type(param);
                    params.push(param_type);
                }
                return_ty = get_type(tp->results);
                fn_ty = llvm::FunctionType::get(return_ty, llvm::ArrayRef(params.data, params.count), false);
                callee = Builder->CreateLoad(Builder->getPtrTy(), decl->backend_var->alloca);
                break;
            }
            }
        } else {
            LLVM_Value call_value = gen_expr(call->elem);
            Type_Proc *tp = (Type_Proc *)call->elem->type;

            Auto_Array<llvm::Type*> params;
            if (tp->params->types.count) {
                for (Type *param : tp->params->types) {
                    llvm::Type *t = get_type(param);
                    params.push(t);
                }
            }
            return_ty = get_type(tp->results);
            fn_ty = llvm::FunctionType::get(return_ty, llvm::ArrayRef(params.data, params.count), false);
            callee = call_value.value;
        }

        Auto_Array<llvm::Value*> args;
        for (int i = 0; i < call->arguments.count; i++) {
            Ast *arg = call->arguments[i];
            LLVM_Value arg_value = gen_expr(arg);

            args.push(arg_value.value);
        }

        llvm::CallInst *call_inst = Builder->CreateCall(fn_ty, callee, llvm::ArrayRef(args.data, args.count));
        value.value = call_inst;
        value.type = return_ty;

        return value;
    }
    
    case AST_SUBSCRIPT: {
        Ast_Subscript *subscript = static_cast<Ast_Subscript*>(expr);
        llvm::Type *type = get_type(subscript->type);
        LLVM_Addr addr = gen_addr(subscript);
        LLVM_Value rhs = gen_expr(subscript->index);
        llvm::LoadInst *load = Builder->CreateLoad(type, addr.value);
        LLVM_Value value;
        value.value = load;
        value.type = type;
        return value;
    }

    case AST_CAST: {
        char *s = (char *)31252;
        Ast_Cast *cast = static_cast<Ast_Cast*>(expr);
        Ast *elem = cast->elem;
        LLVM_Value expr_value = gen_expr(cast->elem);
        llvm::Type* dest_type = get_type(cast->type);

        LLVM_Value value;
        value.type = dest_type;

        if (is_float_type(cast->type) && is_float_type(elem->type)) {
            value.value = Builder->CreateFPCast(expr_value.value, dest_type);
        } else if (is_integral_type(cast->type) && is_integral_type(elem->type)) {
            value.value = Builder->CreateZExtOrTrunc(expr_value.value, dest_type);
            if (cast->type->bytes < elem->type->bytes) value.value = Builder->CreateTrunc(expr_value.value, dest_type);
            else value.value = Builder->CreateIntCast(expr_value.value, dest_type, is_signed_type(cast->type));
        } else if (is_pointer_type(cast->type) && is_pointer_type(elem->type)) {
            value.value = Builder->CreatePointerCast(expr_value.value, dest_type);
        } else if (is_pointer_type(cast->type) && is_integer_type(elem->type)) {
            value.value = Builder->CreateIntToPtr(expr_value.value, dest_type);
        } else if (is_integer_type(cast->type) && is_pointer_type(elem->type)) {
            value.value = Builder->CreatePtrToInt(expr_value.value, dest_type);
        } else {
            // Assert(0);
            value.value = expr_value.value;
        }
        return value;
    }

    case AST_UNARY: {
        Ast_Unary *unary = static_cast<Ast_Unary*>(expr);
        LLVM_Value value = {};
        LLVM_Value ev = gen_expr(unary->elem);
        switch (unary->op) {
        case OP_UNARY_PLUS:
            value.value = ev.value;
            break;
        case OP_UNARY_MINUS:
            value.value = Builder->CreateNeg(ev.value);
            break;
        case OP_BIT_NOT:
            value.value = Builder->CreateNot(ev.value);
            break;
        case OP_NOT:
            value.value = gen_logical_not(ev.value);
            break;
        }
        value.type = get_type(unary->type);
        return value;
    }

    case AST_ADDRESS: {
        Ast_Address *address = static_cast<Ast_Address*>(expr);
        LLVM_Value value;
        LLVM_Addr addr = gen_addr(address->elem);
        value.value = addr.value;
        value.type = get_type(address->type);
        return value;
    }

    case AST_DEREF: {
        Ast_Deref *deref = static_cast<Ast_Deref*>(expr);
        LLVM_Value value;
        LLVM_Addr addr = gen_addr(deref);
        llvm::Type* type = get_type(deref->type);
        llvm::LoadInst *load = Builder->CreateLoad(type, addr.value);
        value.value = load;
        value.type = type;
        return value;
    }

    case AST_BINARY: {
        Ast_Binary *binary = static_cast<Ast_Binary*>(expr);
        return gen_binary_op(binary);
    }

    case AST_SELECTOR: {
        Ast_Selector *selector = static_cast<Ast_Selector*>(expr);
        Decl *decl = selector->name->ref;
        LLVM_Value value = {};

        switch (decl->kind) {
        case DECL_VARIABLE: {
            LLVM_Addr addr = gen_addr(selector);
            llvm::Type *ty = get_type(selector->type);
            llvm::LoadInst *load = Builder->CreateLoad(ty, addr.value);
            value.value = load;
            value.type = ty;
            break;
        }
        default: 
            value = gen_expr(selector->name);
            return value;
        }
        return value;
    }
    }

    return {};
}

LLVM_Addr LLVM_Backend::gen_addr(Ast *expr) {
    LLVM_Addr result = {};

    switch (expr->kind) {
    default:
        Assert(0);
        break;

    case AST_IDENT: {
        Ast_Ident *ident = static_cast<Ast_Ident*>(expr);
        Decl *decl = ident->ref;
        BE_Var *var = decl->backend_var;
        Assert(var);
        result.value = var->value;
        break;
    }

    case AST_SELECTOR: {
        Ast_Selector *selector = static_cast<Ast_Selector*>(expr);
        Ast *base = selector->parent;
        switch (base->mode) {
        case ADDRESSING_TYPE:
            Assert(0);
            break;

        case ADDRESSING_VARIABLE: {
            LLVM_Addr base_addr = gen_addr(base);

            if (base->type->kind == TYPE_STRUCT) {
                Type_Struct *ts = (Type_Struct *)base->type;
                BE_Struct *be_struct = ts->backend_struct;
                Select select = lookup_field(ts, selector->name->name, false);
                llvm::Type *ptr_type = get_type(base->type);
                llvm::Value *access_ptr = Builder->CreateStructGEP(be_struct->type, base_addr.value, (unsigned)select.index);
                result.value = access_ptr;
            } else if (base->type->kind == TYPE_POINTER) {
                Type_Struct *ts = (Type_Struct *)type_deref(base->type);
                BE_Struct *be_struct = ts->backend_struct;
                Select select = lookup_field(ts, selector->name->name, false);
                llvm::Type *ptr_type = get_type(base->type);
                llvm::Value *addr = Builder->CreateLoad(ptr_type, base_addr.value);
                llvm::Value *access_ptr = Builder->CreateStructGEP(be_struct->type, addr, (unsigned)select.index);
                result.value = access_ptr;
            }
            break;
        }
        }
        break;
    }

    case AST_SUBSCRIPT: {
        Ast_Subscript *subscript = static_cast<Ast_Subscript*>(expr);
        LLVM_Addr addr = gen_addr(subscript->expr);
        LLVM_Value rhs = gen_expr(subscript->index);

        if (is_array_type(subscript->expr->type)) {
            llvm::Type *array_type = get_type(subscript->expr->type);
            llvm::ArrayRef<llvm::Value*> indices = {
                llvm_zero(llvm::Type::getInt32Ty(*Ctx)), // ptr
                rhs.value  // subscript
            };
            llvm::Value *ptr = Builder->CreateGEP(array_type, addr.value, indices);
            result.value = ptr;
        } else if (is_pointer_type(subscript->expr->type)) {
            llvm::Type *pointer_type = get_type(subscript->expr->type);
            llvm::PointerType* load_type = llvm::PointerType::get(pointer_type, 0);
            llvm::LoadInst *load = Builder->CreateLoad(load_type, addr.value);
            llvm::Value *ptr = Builder->CreateGEP(pointer_type, load, rhs.value);
            result.value = ptr;
        }
        break;
    }

    case AST_DEREF: {
        Ast_Deref *deref = static_cast<Ast_Deref*>(expr);
        LLVM_Addr addr = gen_addr(deref->elem);
        llvm::Type *pointer_type = get_type(deref->elem->type);
        llvm::LoadInst *load = Builder->CreateLoad(pointer_type, addr.value);
        result.value = load;
        break;
    }
    }

    return result;
}

llvm::Type* LLVM_Backend::get_type(Type *type) {
    switch (type->kind) {
    default:
        Assert(0);
        return nullptr;
    case TYPE_NULL:
        Assert(0); //@Note No expression should still have the null type, needs to have type of "owner"
        return llvm::Type::getVoidTy(*Ctx);

    case TYPE_VOID:
        return llvm::Type::getVoidTy(*Ctx);

    case TYPE_UINT8:
    case TYPE_INT8:
    case TYPE_BOOL:
        return llvm::Type::getInt8Ty(*Ctx);
    case TYPE_UINT16:
    case TYPE_INT16:
        return llvm::Type::getInt16Ty(*Ctx);
    case TYPE_UINT32:
    case TYPE_INT32:
    case TYPE_INT:
    case TYPE_UINT:
        return llvm::Type::getInt32Ty(*Ctx);
    case TYPE_UINT64:
    case TYPE_ISIZE:
    case TYPE_USIZE:
    case TYPE_INT64:
        return llvm::Type::getInt64Ty(*Ctx);

    case TYPE_FLOAT32:
        return llvm::Type::getFloatTy(*Ctx);
    case TYPE_FLOAT64:
        return llvm::Type::getDoubleTy(*Ctx);

    case TYPE_STRING:
        Assert(0);
        return builtin_string_type;

    case TYPE_ENUM: {
        Type_Enum *te = static_cast<Type_Enum*>(type);
        return get_type(te->base_type);
    }

    case TYPE_STRUCT: {
        Type_Struct *ts = (Type_Struct *)type;
        BE_Struct *bs = ts->backend_struct;
        if (!bs) {
            gen_type_struct(ts);
            bs = ts->backend_struct;
        }
        return bs->type;
    }

    case TYPE_ARRAY: {
        Type_Array *array_type = static_cast<Type_Array*>(type);
        llvm::Type* element_type = get_type(array_type->base);
        llvm::ArrayType *array_typeref = llvm::ArrayType::get(element_type, array_type->array_size);
        return array_typeref;
    }

    case TYPE_POINTER:
        return llvm::PointerType::get(*Ctx, 0);

    case TYPE_PROC: {
        Type_Proc *proc_ty = static_cast<Type_Proc*>(type);
        llvm::Type *results = get_type(proc_ty->results);
        Type_Tuple *params = proc_ty->params;
        Auto_Array<llvm::Type*> parameter_types;
        for (Type *param : params->types) {
            llvm::Type *param_type = get_type(param);
            parameter_types.push(param_type);
        }
        llvm::FunctionType *function_type = llvm::FunctionType::get(results, llvm::ArrayRef(parameter_types.data, parameter_types.count), false);
        llvm::PointerType *pointer_type = llvm::PointerType::get(function_type, 0);
        return pointer_type;
    }

    case TYPE_TUPLE: {
        Type_Tuple *tuple = static_cast<Type_Tuple*>(type);
        if (tuple->types.count == 0) {
            return llvm::Type::getVoidTy(*Ctx);
        } else if (tuple->types.count == 1) {
            return get_type(tuple->types[0]);
        } else {
            Auto_Array<llvm::Type*> types = {};
            for (Type *ty : tuple->types) {
                llvm::Type *t = get_type(ty);
                types.push(t);
            }
            bool is_packed = true;
            llvm::StructType *st = llvm::StructType::create(llvm::ArrayRef(types.data, types.count), "", is_packed);
            return st;
        }
    }
    }
}


void LLVM_Backend::gen_statement_list(Auto_Array<Ast*> statement_list) {
    for (Ast *stmt : statement_list) {
        gen_stmt((Ast_Stmt *)stmt);
    }
}

void LLVM_Backend::gen_block(Ast_Block *block) {
    gen_statement_list(block->statements);
}

llvm::Value* LLVM_Backend::gen_condition(Ast *expr) {
    LLVM_Value value = gen_expr(expr);

    llvm::Value* cond = nullptr;
    if (is_boolean_type(expr->type)) {
        cond = value.value;
    } else {
        cond = Builder->CreateICmpNE(value.value, llvm::Constant::getNullValue(value.type));
    }
    llvm::Value *result = Builder->CreateTrunc(cond, llvm::Type::getInt1Ty(*Ctx));
    return result;
}

void LLVM_Backend::gen_if(Ast_If *if_stmt) {
    // if x { ... }
    // else if y { ... }
    // ......
    // else if z { ... }
    // else { ... }
    // gen cond
    // icmp neq cond, 0
    // br icmp, if_then, if_else
    // or
    // br if_then -- if no else
    // label: if_then
    // build if block
    /// br if_exit
    // label: if_else
    // build else stmt
    // br if_exit
    // label: if_exit

    llvm::BasicBlock *exit_block = llvm_block_new("if.exit");

    for (Ast_If *current = if_stmt; current; current = (Ast_If *)current->next) {
        Ast_If *else_stmt = (Ast_If *)current->next;

        llvm::BasicBlock *then_block = llvm_block_new("if.then");
        llvm::BasicBlock *else_block = exit_block;

        if (else_stmt) {
            else_block = llvm_block_new("if.else");
        }

        if (current->is_else) {
            gen_branch(then_block);
        } else {
            llvm::Value *cond = gen_condition(current->cond);
            gen_branch_condition(cond, then_block, else_block);
        }

        emit_block(then_block);
        gen_block(current->block);

        gen_branch(exit_block);

        emit_block(else_block);
    }
}

void LLVM_Backend::gen_ifcase_switch(Ast_Ifcase *ifcase) {
    Assert(ifcase->cond);
    Assert(is_integral_type(ifcase->cond->type));
    LLVM_Value condition = gen_expr(ifcase->cond);

    llvm::BasicBlock *switch_exit = (llvm::BasicBlock *)ifcase->exit_block;
    llvm::BasicBlock *default_block = switch_exit;
    if (ifcase->default_case) {
        default_block = (llvm::BasicBlock *)ifcase->default_case->backend_block;
    }

    llvm::SwitchInst *switch_inst = llvm::SwitchInst::Create(condition.value, default_block, (unsigned)ifcase->cases.count);

    for (Ast_Case_Label *label : ifcase->cases) {
        if (label->is_default) continue;
        if (label->cond->kind == AST_RANGE) {
            Ast_Range *range = static_cast<Ast_Range*>(label->cond);
            llvm::Type *type = get_type(range->type);
            bigint max = range->rhs->value.value_integer;
            bigint i = bigint_copy(&range->lhs->value.value_integer);

            while (mp_cmp(&i, &max) == MP_LT) {
                bool is_signed = mp_isneg(&i);
                u64 c = u64_from_bigint(i);
                llvm::ConstantInt *case_constant = static_cast<llvm::ConstantInt*>(llvm::ConstantInt::get(type, c, is_signed));
                switch_inst->addCase(case_constant, (llvm::BasicBlock *)label->backend_block);
            }
        } else {
            LLVM_Value case_cond = gen_expr(label->cond);
            llvm::ConstantInt *case_constant = static_cast<llvm::ConstantInt*>(case_cond.value);
            switch_inst->addCase(case_constant, (llvm::BasicBlock *)label->backend_block);
        }
    }

    Builder->Insert(switch_inst);
}

void LLVM_Backend::gen_ifcase_if_else(Ast_Ifcase *ifcase) {
    LLVM_Value condition = gen_expr(ifcase->cond);
    llvm::BasicBlock *switch_exit = (llvm::BasicBlock *)ifcase->exit_block;
    llvm::BasicBlock *jump_exit = llvm_block_new();

    //@Note If-Else branching
    for (Ast_Case_Label *label : ifcase->cases) {
        // Skip default to ensure it is only branched at end of if-else chain
        if (label->is_default) continue;
        Ast_Case_Label *else_label = static_cast<Ast_Case_Label*>(label->next);
        if (else_label && else_label->is_default) {
            else_label = static_cast<Ast_Case_Label*>(else_label->next);
        }

        llvm::BasicBlock *else_block = jump_exit;
        if (else_label) {
            else_block = llvm_block_new();
        }

        llvm::Value *compare = nullptr;
        if (label->cond->kind == AST_RANGE) {
            //@Note Check if condition is within range
            Ast_Range *range = static_cast<Ast_Range*>(label->cond);
            LLVM_Value lhs = gen_expr(range->lhs);
            LLVM_Value rhs = gen_expr(range->rhs);

            llvm::Value *gte = Builder->CreateICmp(llvm::CmpInst::ICMP_SGE, condition.value, lhs.value);
            llvm::Value *lte = Builder->CreateICmp(llvm::CmpInst::ICMP_SLE, condition.value, rhs.value);
            compare = Builder->CreateAnd(gte, lte);
        } else {
            if (ifcase->cond) {
                LLVM_Value label_cond = gen_expr(label->cond);
                compare = Builder->CreateICmp(llvm::CmpInst::ICMP_EQ, condition.value, label_cond.value);
            } else {
                llvm::Value *value = gen_condition(label->cond);
                compare = value;
            }
        }

        gen_branch_condition(compare, (llvm::BasicBlock *)label->backend_block, else_block);

        emit_block(else_block);
    }

    llvm::BasicBlock *default_block = (llvm::BasicBlock *)ifcase->exit_block;
    if (ifcase->default_case) {
        default_block = (llvm::BasicBlock *)ifcase->default_case->backend_block;
    }
    gen_branch(default_block);
}

void LLVM_Backend::gen_ifcase(Ast_Ifcase *ifcase) {
    if (ifcase->cases.count == 0) return;

    ifcase->exit_block = llvm_block_new();
    llvm::BasicBlock *switch_exit = (llvm::BasicBlock *)ifcase->exit_block;

    for (Ast_Case_Label *label = ifcase->cases.front(); label; label = static_cast<Ast_Case_Label *>(label->next)) {
        Ast_Case_Label *prev = static_cast<Ast_Case_Label*>(label->prev);
        if (!prev) {
            label->backend_block = llvm_block_new();
            continue;
        }

        if (prev->statements.count == 0) {
            label->backend_block = prev->backend_block;
        } else {
            label->backend_block = llvm_block_new();
        }
    }

    if (ifcase->switchy) {
        gen_ifcase_switch(ifcase);
    } else {
        gen_ifcase_if_else(ifcase);
    }

    current_block = nullptr;

    llvm::BasicBlock *case_block = nullptr;
    for (Ast_Case_Label *label : ifcase->cases) {
        //@Note Skip default to ensure default is last on if-else
        if (!ifcase->switchy && label->is_default) {
            continue;
        }

        if (case_block != label->backend_block) {
            case_block = (llvm::BasicBlock *)label->backend_block;
            emit_block(case_block);
        }

        if (label->statements.count == 0) continue;

        gen_statement_list(label->statements);

        llvm::BasicBlock *next_block = switch_exit;
        gen_branch(next_block);
    }

    if (!ifcase->switchy && ifcase->default_case) {
        emit_block((llvm::BasicBlock *)ifcase->default_case->backend_block);
        for (Ast *stmt : ifcase->default_case->statements) {
            // if (stmt->is_stmt()) {
                gen_stmt((Ast_Stmt *)stmt);
            // } else if (stmt->is_decl()) {
                // gen_decl((Ast_Decl *)stmt);
            // }
        }
        gen_branch(switch_exit);
    }

    emit_block(switch_exit);
}

void LLVM_Backend::gen_while(Ast_While *while_stmt) {
    // while x { ... }
    // label loop_head:
    // gen x
    // ifcmp neq x, 0
    // br loop_tail, loop_body
    // label loop_body:
    // ...
    // br loop_head
    // label loop_tail:

    llvm::BasicBlock *body = llvm_block_new();
    while_stmt->entry_block = llvm_block_new();
    while_stmt->exit_block = llvm_block_new();

    gen_branch((llvm::BasicBlock *)while_stmt->entry_block);
    emit_block((llvm::BasicBlock *)while_stmt->entry_block);

    llvm::Value* condition = gen_condition(while_stmt->cond);
    gen_branch_condition(condition, body, (llvm::BasicBlock*)while_stmt->exit_block);

    emit_block(body);
    gen_block(while_stmt->block);
    gen_branch((llvm::BasicBlock *)while_stmt->entry_block);

    emit_block((llvm::BasicBlock *)while_stmt->exit_block);
}

llvm::Value *LLVM_Backend::llvm_struct_gep(llvm::Type *type, llvm::Value *ptr, unsigned index) {
    llvm::Value *value = Builder->CreateStructGEP(type, ptr, index);
    return value;
}

void LLVM_Backend::llvm_store(llvm::Value *value, llvm::Value *ptr) {
    Builder->CreateStore(value, ptr);
}

void LLVM_Backend::emit_jump(llvm::BasicBlock *target) {
    gen_branch(target);
    llvm::BasicBlock *next_block = llvm_block_new("unreachable");
    emit_block(next_block);
}

void LLVM_Backend::gen_break(Ast_Break *break_stmt) {
    Ast *ast = break_stmt->target;
    Assert(ast);
    llvm::BasicBlock *next_block = nullptr;
    switch (ast->kind) {
    case AST_WHILE: {
        Ast_While *while_stmt = static_cast<Ast_While*>(ast);
        next_block = (llvm::BasicBlock *)while_stmt->exit_block;
        break;
    }
    case AST_FOR: {
        Ast_For *for_stmt = static_cast<Ast_For*>(ast);
        next_block = (llvm::BasicBlock *)for_stmt->exit_block;
        break;
    }
    case AST_IFCASE: {
        Ast_Ifcase *ifcase = static_cast<Ast_Ifcase*>(ast);
        next_block = (llvm::BasicBlock *)ifcase->exit_block;
        break;
    }
    }
    emit_jump(next_block);
}

void LLVM_Backend::gen_continue(Ast_Continue *continue_stmt) {
    Ast *ast = continue_stmt->target;
    Assert(ast);

    llvm::BasicBlock *next_block = nullptr;
    switch (ast->kind) {
    case AST_WHILE: {
        Ast_While *while_stmt = static_cast<Ast_While*>(ast);
        next_block = (llvm::BasicBlock *)while_stmt->entry_block;
        break;
    }
    case AST_FOR: {
        Ast_For *for_stmt = static_cast<Ast_For*>(ast);
        next_block = (llvm::BasicBlock *)for_stmt->entry_block;
        break;
    }
    }
    emit_jump(next_block);
}

void LLVM_Backend::gen_fallthrough(Ast_Fallthrough *fallthrough) {
    Assert(fallthrough->target->kind == AST_CASE_LABEL);
    Ast_Case_Label *label = static_cast<Ast_Case_Label*>(fallthrough->target);
    if (label->next) {
        Ast_Case_Label *next = static_cast<Ast_Case_Label*>(label->next);
        emit_jump((llvm::BasicBlock *)next->backend_block);
    }
}

void LLVM_Backend::gen_return(Ast_Return *return_stmt) {
    if (return_stmt->values.count) {
        llvm::Value *retval = current_proc->return_value;

        if (return_stmt->values.count == 1) {
            LLVM_Value value = gen_expr(return_stmt->values[0]);
            llvm_store(value.value, retval);
        } else {
            for (int i = 0; i < return_stmt->values.count; i++) {
                LLVM_Value value = gen_expr(return_stmt->values[i]);
                llvm::Value* addr = Builder->CreateStructGEP(current_proc->results, retval, (unsigned)i);
                Builder->CreateStore(value.value, addr);
            }
        }
    }

    emit_jump(current_proc->exit_block);
}

void LLVM_Backend::gen_stmt(Ast *stmt) {
    switch (stmt->kind) {
    case AST_EXPR_STMT: {
        Ast_Expr_Stmt *expr_stmt = static_cast<Ast_Expr_Stmt*>(stmt);
        Ast *expr = expr_stmt->expr;
        LLVM_Value value = gen_expr(expr);
        break;
    }

    case AST_ASSIGNMENT: {
        Ast_Assignment *assignment = static_cast<Ast_Assignment*>(stmt);
        gen_assignment_stmt(assignment);
        break;
    }

    case AST_VALUE_DECL: {
        Ast_Value_Decl *vd = static_cast<Ast_Value_Decl*>(stmt);
        // gen_value_decl(vd);
        break;
    }

    case AST_IF: {
        Ast_If *if_stmt = static_cast<Ast_If*>(stmt);
        gen_if(if_stmt);
        break;
    }

    case AST_IFCASE: {
        Ast_Ifcase *ifcase = static_cast<Ast_Ifcase*>(stmt);
        gen_ifcase(ifcase);;
        break;
    }

    case AST_WHILE: {
        Ast_While *while_stmt = static_cast<Ast_While*>(stmt);
        gen_while(while_stmt);
        break;
    }

    case AST_FOR: {
        Ast_For *for_stmt = static_cast<Ast_For*>(stmt);
        // gen_for(for_stmt);
        break;
    }

    case AST_BLOCK: {
        Ast_Block *block = static_cast<Ast_Block*>(stmt);
        gen_block(block);
        break;
    }

    case AST_BREAK: {
        Ast_Break *break_stmt = static_cast<Ast_Break *>(stmt);
        gen_break(break_stmt);
        break;
    }

    case AST_CONTINUE: {
        Ast_Continue *continue_stmt = static_cast<Ast_Continue*>(stmt);
        gen_continue(continue_stmt);
        break;
    }

    case AST_RETURN: {
        Ast_Return *return_stmt = static_cast<Ast_Return*>(stmt);
        gen_return(return_stmt);
        break;
    }

    case AST_FALLTHROUGH: {
        Ast_Fallthrough *fallthrough = static_cast<Ast_Fallthrough*>(stmt);
        gen_fallthrough(fallthrough);
        break;
    }
    }
}


void LLVM_Backend::gen_type_struct(Type_Struct *ts) {
    if (ts->backend_struct) {
        return;
    }

    Atom *name = ts->name;

    BE_Struct *be_struct = llvm_alloc(BE_Struct);
    be_struct->name = name;
    ts->backend_struct = be_struct;

    llvm::StructType *struct_type = llvm::StructType::create(*Ctx, name->data);
    be_struct->type = struct_type;

    for (Decl *member : ts->members) {
        if (member->kind != DECL_PROCEDURE) gen_decl(member);

        if (member->kind == DECL_VARIABLE) {
            llvm::Type* member_type = get_type(member->type);
            be_struct->element_types.push(member_type);
        }
    }
    struct_type->setBody(llvm::ArrayRef(be_struct->element_types.data, be_struct->element_types.count), false);
}

void LLVM_Backend::gen_decl_variable(Decl *decl) {
}

void LLVM_Backend::gen_decl_procedure(Decl *decl) {
    Ast_Proc_Lit *proc_lit = decl->proc_lit;
    
    BE_Proc *be_proc = llvm_alloc(BE_Proc);
    be_proc->name = decl->name;
    be_proc->proc_lit = proc_lit;
    proc_lit->backend_proc = be_proc;

    Type_Proc *tp = (Type_Proc *)proc_lit->type;

    for (Type *param : tp->params->types) {
        // if (param->is_vararg) break;
        be_proc->params.push(get_type(param));
    }

    be_proc->results = get_type(tp->results);

    bool variadic = false;

    llvm::FunctionType *function_type = llvm::FunctionType::get(be_proc->results, llvm::ArrayRef(be_proc->params.data, be_proc->params.count), variadic);
    be_proc->type = function_type;

    llvm::Function *fn = llvm::Function::Create(function_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, 0, decl->name->data, Module);
    be_proc->fn = fn;

    // if (proc->foreign) {
    //     fn->setCallingConv(llvm::CallingConv::C);
    // }
    // if (proc->has_varargs) {
    //     fn->setCallingConv(llvm::CallingConv::C);
    // }
    // if (!proc->foreign) {
    // }

    if (proc_lit->body) {
        llvm::BasicBlock *entry = llvm_block_new();
        be_proc->builder = new llvm::IRBuilder<>(*Ctx);
        be_proc->entry = entry;

        llvm::BasicBlock *exit_block = llvm_block_new();
        be_proc->exit_block = exit_block;
    }
}

void LLVM_Backend::set_procedure(BE_Proc *procedure) {
    current_proc = procedure;
    Builder = procedure->builder;
}

void LLVM_Backend::gen_procedure_body(Decl *proc_decl) {
    Ast_Proc_Lit *proc_lit = proc_decl->proc_lit;
    if (!proc_lit) return;

    BE_Proc *procedure = proc_lit->backend_proc;

    set_procedure(procedure);

    emit_block(procedure->entry);

    if (!procedure->results->isVoidTy()) {
        procedure->return_value = Builder->CreateAlloca(procedure->results, 0, nullptr);
        procedure->return_value->setName(".RET");
    }

    int arg_idx = 0;
    for (Ast *var : proc_lit->local_vars) {
        switch (var->kind) {
        case AST_PARAM: {
            Ast_Param *param = static_cast<Ast_Param*>(var);
            Decl *decl = param->name->ref;
            BE_Var *var = llvm_alloc(BE_Var);
            decl->backend_var = var;
            var->name = decl->name;
            var->decl = decl;
            var->type = get_type(decl->type);
            var->value = Builder->CreateAlloca(var->type, 0, nullptr);
            llvm::Argument *argument = procedure->fn->getArg(arg_idx);
            arg_idx++;
            llvm_store(argument, var->alloca);
            break;
        }

        case AST_VALUE_DECL: {
            Ast_Value_Decl *vd = static_cast<Ast_Value_Decl*>(var);
            gen_value_decl(vd);
            break;
        }
        }
    }

    gen_block(proc_lit->body);

    //@Note Do not branch to exit block if already branch to exit block
    gen_branch(procedure->exit_block);

    emit_block(procedure->exit_block);
    if (procedure->results->isVoidTy()) {
        llvm::ReturnInst *ret = Builder->CreateRetVoid();
    } else {
        llvm::Value *value = Builder->CreateLoad(procedure->results, procedure->return_value);
        llvm::ReturnInst *ret = Builder->CreateRet(value);
    }

    current_block = nullptr;
}

void LLVM_Backend::gen_decl(Decl *decl) {
    switch (decl->kind) {
    case DECL_TYPE: {
        switch (decl->type->kind) {
        case TYPE_STRUCT: {
            Type_Struct *ts = (Type_Struct *)decl->type;
            gen_type_struct(ts);
            break;
        }
        case TYPE_ENUM:
        case TYPE_UNION:
            break;
        }
        break;
    }
    case DECL_VARIABLE:
        // gen_decl_variable(decl);
        break;

    case DECL_PROCEDURE:
        gen_decl_procedure(decl);
        break;
    }
}

void LLVM_Backend::gen_value_decl(Ast_Value_Decl *vd) {
    bool is_global = vd->flags & AST_FLAG_GLOBAL;

    if (vd->is_mutable) {
        for (Ast *name : vd->names) {
            Ast_Ident *ident = (Ast_Ident *)name;
            Decl *decl = ident->ref;

            if (decl->kind == DECL_VARIABLE) {
                BE_Var *var = llvm_alloc(BE_Var);
                decl->backend_var = var;
                var->name = ident->name;
                var->decl = decl;
                var->type = get_type(decl->type);
                if (is_global) {
                    llvm::GlobalVariable *global_variable = new llvm::GlobalVariable(*Module, var->type, false, llvm::GlobalValue::ExternalLinkage, nullptr, (char *)ident->name->data);
                    var->value = global_variable;
                } else {
                    llvm::AllocaInst *alloca = Builder->CreateAlloca(var->type, 0, nullptr);
                    var->value = alloca;
                    current_proc->named_values.push(var);
                }
                var->value->setName((char *)var->name->data);
            }
        }

        for (int v = 0, n = 0; v < vd->values.count; v++) {
            Ast *val_expr = vd->values[v];

            LLVM_Value value = gen_expr(val_expr);

            if (is_global) {
                Ast *name = vd->names[n];
                Ast_Ident *ident = (Ast_Ident *)name;
                Decl *decl = ident->ref;
                BE_Var *var = decl->backend_var;
                llvm::GlobalVariable *global_variable = (llvm::GlobalVariable *)var->value;
                llvm::Constant *init = (llvm::Constant *)value.value;
                global_variable->setInitializer(init);
                n++;
            } else {
                int value_count = get_value_count(val_expr->type);

                if (value_count == 1) {
                    Ast_Ident *ident = (Ast_Ident *)vd->names[n];
                    BE_Var *var = ident->ref->backend_var;
                    llvm_store(value.value, var->value);
                } else {
                    llvm::Type *type = get_type(val_expr->type);

                    for (int i = 0; i < value_count; i++, n++) {
                        Ast *name = vd->names[n];
                        Ast_Ident *ident = (Ast_Ident *)name;
                        Decl *decl = ident->ref;

                        if (decl->kind == DECL_VARIABLE) {
                            BE_Var *var = decl->backend_var;
                            Type_Tuple *tup = (Type_Tuple *)val_expr->type;
                            llvm::Value *ptr = llvm_struct_gep(type, var->value, (unsigned)i);
                            llvm_store(value.value, ptr);
                        }
                    }
                }
                n += value_count;
            }
        }
    } else {
        for (Ast *name : vd->names) {
            Ast_Ident *ident = (Ast_Ident *)name;
            Decl *decl = ident->ref;
            gen_decl(decl);
        }
    }
}

void LLVM_Backend::gen() {
    Ctx = new llvm::LLVMContext();
    Module = new llvm::Module("Main", *Ctx);
    Builder = new llvm::IRBuilder<>(*Ctx);

    for (Ast *decl : ast_root->decls) {
        if (decl->kind == AST_VALUE_DECL) {
            gen_value_decl((Ast_Value_Decl *)decl);
        }
    }

    for (Decl *decl : ast_root->scope->decls) {
        if (decl->kind == DECL_PROCEDURE) {
            gen_procedure_body(decl);
        }
    }

    std::error_code EC;
    llvm::raw_fd_ostream OS("-", EC);
    bool broken_debug_info;
    if (llvm::verifyModule(*Module, &OS, &broken_debug_info)) {
        Module->print(llvm::errs(), nullptr); // dump IR
        return;
    }

    if (compiler_dump_IR) {
        Module->print(llvm::errs(), nullptr); // dump IR
    }

    LLVMInitializeNativeTarget();
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86Disassembler();
    LLVMInitializeX86AsmPrinter();
    LLVMInitializeX86AsmParser();

    char *errors = nullptr;

	char const *target_triple = LLVM_DEFAULT_TARGET_TRIPLE;
    LLVMTargetRef target;
    if (LLVMGetTargetFromTriple(target_triple, &target, &errors)) {
        fprintf(stderr, "ERROR:%s\n", errors);
        return;
    }
    LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(target, target_triple, "generic", LLVMGetHostCPUFeatures(), LLVMCodeGenLevelDefault, LLVMRelocDefault, LLVMCodeModelDefault);
    LLVMSetTarget((LLVMModuleRef)Module, target_triple);

    LLVMTargetDataRef data_layout = LLVMCreateTargetDataLayout(target_machine);
    char* data_layout_str = LLVMCopyStringRepOfTargetData(data_layout);
    LLVMSetDataLayout((LLVMModuleRef)Module, data_layout_str);
    LLVMDisposeMessage(data_layout_str);

    String8 object_name = path_remove_extension(path_file_name(file->path));
    String8 object_file_name = path_join(heap_allocator(), os_current_dir(heap_allocator()), object_name);
    object_file_name = str8_concat(heap_allocator(), object_file_name, str_lit(".o"));

    if (LLVMTargetMachineEmitToFile(target_machine, (LLVMModuleRef)Module, (char *)object_file_name.data, LLVMObjectFile, &errors)) {
        fprintf(stderr, "ERROR:%s\n", errors);
        return;
    }

    compiler_link_libraries.push(str8_lit("msvcrt.lib"));
    compiler_link_libraries.push(str8_lit("legacy_stdio_definitions.lib"));
    char *linker_args = NULL;
    for (String8 lib : compiler_link_libraries) {
        linker_args = cstring_append_fmt(linker_args, "%S ", lib);
    }
    char *linker_command = cstring_fmt("link.exe %s %s", (char *)object_file_name.data, linker_args);

    printf("LINK COMMAND: %s\n", linker_command);

    system(linker_command);
}
