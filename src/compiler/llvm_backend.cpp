global Arena *llvm_arena;

#define llvm_alloc(T) (T*)llvm_backend_alloc(sizeof(T), alignof(T))
internal void *llvm_backend_alloc(u64 size, int alignment) {
    void *result = (void *)arena_alloc(llvm_arena, size, alignment);
    MemoryZero(result, size);
    return result;
}

internal unsigned llvm_get_struct_field_index(Ast_Struct *struct_decl, Atom *name) {
    for (unsigned i = 0; i < struct_decl->fields.count; i++) {
        Ast_Struct_Field *field = struct_decl->fields[i];
        if (atoms_match(field->name, name)) {
            return i;
        }
    }
    Assert(0);
    return 0;
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

// llvm::Value *LLVM_Backend::emit_gep_offset(llvm::Value *ptr, llvm::Value *offset) {
    // Assert(ptr->getType()->isPointerTy());
    // llvm::PointerType *type = static_cast<llvm::PointerType*>(ptr->getType());
    // llvm::Value *result = builder->CreateGEP(type->getElementType(), ptr, offset);
    // return result;
// }

llvm::Value *LLVM_Backend::gen_logical_not(llvm::Value *value) {
    llvm::Type *type = value->getType();
    auto size_in_bits = type->getPrimitiveSizeInBits();
    if (type->isFloatingPointTy()) {
        return builder->CreateFCmpOEQ(value, llvm::ConstantFP::get(builder->getFloatTy(), 0.0));
    } else if (type->isSingleValueType()) {
        return builder->CreateICmpEQ(value, llvm::ConstantInt::get(builder->getIntNTy((unsigned)size_in_bits), 0, false));
    }
    return value;
}

void LLVM_Backend::emit_block(llvm::BasicBlock *block) {
    Assert(current_block == NULL);
    llvm::Function *fn = current_proc->fn;
    fn->insert(fn->end(), block);
    builder->SetInsertPoint(block);
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
    builder->CreateCondBr(condition, true_block, false_block);
}

void LLVM_Backend::gen_branch(llvm::BasicBlock *target_block) {
    if (!emit_block_check_branch()) {
        return;
    }
    current_block = nullptr;
    builder->CreateBr(target_block);
}

LLVM_Procedure *LLVM_Backend::gen_procedure(Ast_Proc *proc) {
    LLVM_Procedure *llvm_proc = llvm_alloc(LLVM_Procedure);
    llvm_proc->name = proc->name;
    llvm_proc->proc = proc;

    Ast_Proc_Type_Info *proc_type_info = static_cast<Ast_Proc_Type_Info*>(proc->type_info);

    for (int i = 0; i < proc->parameters.count; i++) {
        Ast_Param *param = proc->parameters[i];
        if (param->is_vararg) break;
        Ast_Type_Info *type_info = proc_type_info->parameters[i];
        llvm::Type* type = get_type(type_info);
        llvm_proc->parameter_types.push(type);
    }
    llvm::Type *return_type = get_type(proc_type_info->return_type);
    llvm_proc->return_type = return_type;

    llvm::FunctionType *function_type = llvm::FunctionType::get(return_type, llvm::ArrayRef(llvm_proc->parameter_types.data, llvm_proc->parameter_types.count), proc->has_varargs);
    llvm_proc->type = function_type;

    llvm::Function *fn = llvm::Function::Create(function_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, 0, proc->name->data, Module);
    llvm_proc->fn = fn;

    if (proc->foreign) {
        fn->setCallingConv(llvm::CallingConv::C);
    }

    if (proc->has_varargs) {
        fn->setCallingConv(llvm::CallingConv::C);
    }

    if (!proc->foreign) {
        llvm::BasicBlock *entry = llvm_block_new();
        llvm_proc->builder = new llvm::IRBuilder<>(*Ctx);
        llvm_proc->entry = entry;

        llvm::BasicBlock *exit_block = llvm_block_new();
        llvm_proc->exit_block = exit_block;
    }

    return llvm_proc;
}

void LLVM_Backend::set_procedure(LLVM_Procedure *procedure) {
    current_proc = procedure;
    builder = procedure->builder;
}

void LLVM_Backend::gen_procedure_body(LLVM_Procedure *procedure) {
    Ast_Proc *proc = procedure->proc;

    if (proc->foreign) {
        return;
    }

    set_procedure(procedure);

    emit_block(procedure->entry);

    if (!procedure->return_type->isVoidTy()) {
        procedure->return_value = builder->CreateAlloca(procedure->return_type, 0, nullptr);
    }

    for (int i = 0; i < proc->local_vars.count; i++) {
        Ast_Decl *node = proc->local_vars[i];
        LLVM_Var *var = llvm_alloc(LLVM_Var);
        var->name = node->name;
        var->decl = node;
        var->type = get_type(node->type_info);
        var->alloca = builder->CreateAlloca(var->type, 0, nullptr);
        procedure->named_values.push(var);
        node->backend_var = (void *)var;
    }

    // Store value of arg to alloca
    for (int i = 0; i < proc->parameters.count; i++) {
        LLVM_Var *var = procedure->named_values[i];
        Assert(var->decl->kind == AST_PARAM);
        llvm::Argument *argument = procedure->fn->getArg(i);
        llvm_store(argument, var->alloca);
    }

    gen_block(proc->block);

    //@Note Do not branch to exit block if already branch to exit block
    // if (!llvm_get_last_instruction(procedure->fn)->isTerminator()) {
        gen_branch(procedure->exit_block);
    // }

    emit_block(procedure->exit_block);
    if (procedure->return_type->isVoidTy()) {
        llvm::ReturnInst *ret = builder->CreateRetVoid();
    } else {
        llvm::Value *value = builder->CreateLoad(procedure->return_type, procedure->return_value);
        llvm::ReturnInst *ret = builder->CreateRet(value);
    }

    current_block = nullptr;
}

LLVM_Struct *LLVM_Backend::gen_struct(Ast_Struct *struct_decl) {
    LLVM_Struct *llvm_struct = llvm_alloc(LLVM_Struct);
    llvm_struct->name = struct_decl->name;

    llvm::StructType *struct_type = llvm::StructType::create(*Ctx, llvm_struct->name->data);
    llvm_struct->type = struct_type;

    if (struct_decl->fields.count) {
        llvm_struct->element_types.reserve(struct_decl->fields.count);
        for (int i = 0; i < struct_decl->fields.count; i++) {
            Ast_Struct_Field *field = struct_decl->fields[i];
            llvm::Type* field_type = get_type(field->type_info);
            llvm_struct->element_types.push(field_type);
        }
        struct_type->setBody(llvm::ArrayRef(llvm_struct->element_types.data, llvm_struct->element_types.count), false);
    }

    return llvm_struct;
}

LLVM_Procedure *LLVM_Backend::lookup_proc(Atom *name) {
    for (int i = 0; i < global_procedures.count; i++) {
        LLVM_Procedure *proc = global_procedures[i];
        if (atoms_match(proc->name, name)) {
            return proc;
        }
    }
    return NULL;
}

LLVM_Struct *LLVM_Backend::lookup_struct(Atom *name) {
    for (int i = 0; i < global_structs.count; i++) {
        LLVM_Struct *s = global_structs[i];
        if (atoms_match(s->name, name)) {
            return s;
        }
    }
    return NULL;
}

llvm::Type* LLVM_Backend::get_type(Ast_Type_Info *type_info) {
    if (type_info->type_flags & TYPE_FLAG_BUILTIN) {
        switch (type_info->builtin_kind) {
        default:
            Assert(0);
            return nullptr;
        case BUILTIN_TYPE_NULL:
            Assert(0); //@Note No expression should still have the null type, needs to have type of "owner"
            return llvm::Type::getVoidTy(*Ctx);
        case BUILTIN_TYPE_VOID:
            return llvm::Type::getVoidTy(*Ctx);
        case BUILTIN_TYPE_U8:
        case BUILTIN_TYPE_BOOL:
        case BUILTIN_TYPE_I8:
            return llvm::Type::getInt8Ty(*Ctx);
        case BUILTIN_TYPE_U16:
        case BUILTIN_TYPE_I16:
            return llvm::Type::getInt16Ty(*Ctx);
        case BUILTIN_TYPE_U32:
        case BUILTIN_TYPE_I32:
        case BUILTIN_TYPE_INT:
            return llvm::Type::getInt32Ty(*Ctx);
        case BUILTIN_TYPE_U64:
        case BUILTIN_TYPE_ISIZE:
        case BUILTIN_TYPE_USIZE:
        case BUILTIN_TYPE_I64:
            return llvm::Type::getInt64Ty(*Ctx);
        case BUILTIN_TYPE_F32:
            return llvm::Type::getFloatTy(*Ctx);
        case BUILTIN_TYPE_F64:
            return llvm::Type::getDoubleTy(*Ctx);
        case BUILTIN_TYPE_STRING:
            return builtin_string_type;
        }
    } else if (type_info->is_enum_type()) {
        llvm::Type *type = get_type(type_info->base);
        return type;
    } else if (type_info->is_struct_type()) {
        LLVM_Struct *llvm_struct = lookup_struct(type_info->decl->name);
        return llvm_struct->type;
    } else if (type_info->is_array_type()) {
        Ast_Array_Type_Info *array_type_info = static_cast<Ast_Array_Type_Info*>(type_info);
        llvm::Type* element_type = get_type(array_type_info->base);
        llvm::ArrayType *array_type = llvm::ArrayType::get(element_type, array_type_info->array_size);
        return array_type;
    } else if (type_info->is_pointer_type()) {
        llvm::Type* element_type = get_type(type_info->base);
        llvm::PointerType* pointer_type = llvm::PointerType::get(element_type, 0);
        return pointer_type;
    } else {
        Assert(0);
        return nullptr;
    }
}

LLVM_Addr LLVM_Backend::gen_addr(Ast_Expr *expr) {
    LLVM_Addr result = {};

    switch (expr->kind) {
    default:
        Assert(0);
        break;

    case AST_IDENT:
    {
        Ast_Ident *ident = static_cast<Ast_Ident*>(expr);
        LLVM_Var *var = (LLVM_Var *)ident->decl->backend_var;
        Assert(var);
        result.value = var->alloca;
        break;
    }

    case AST_ACCESS:
    {
        Ast_Access *access = static_cast<Ast_Access*>(expr);

        if (access->parent->type_info->is_struct_type()) {
            LLVM_Addr addr = gen_addr(access->parent);
            Ast_Struct *struct_node = static_cast<Ast_Struct*>(access->parent->type_info->decl);
            LLVM_Struct *llvm_struct = lookup_struct(struct_node->name);
            unsigned idx = llvm_get_struct_field_index(struct_node, access->name->name);
            llvm::Value* access_ptr_value = builder->CreateStructGEP(llvm_struct->type, addr.value, idx);
            result.value = access_ptr_value;
        } else if (access->parent->type_info->is_pointer_type()) {
            LLVM_Addr addr = gen_addr(access->parent);
            Ast_Type_Info *struct_type = access->parent->type_info->base;
            Ast_Struct *struct_node = static_cast<Ast_Struct*>(struct_type->decl);
            LLVM_Struct *llvm_struct = lookup_struct(struct_node->name);
            Struct_Field_Info *struct_field = struct_lookup(struct_type, access->name->name);
            unsigned field_idx = llvm_get_struct_field_index(struct_node, access->name->name);
            llvm::Type *ptr_type = get_type(access->parent->type_info);
            llvm::Value *base_addr = builder->CreateLoad(ptr_type, addr.value);
            llvm::Value *access_ptr = builder->CreateStructGEP(llvm_struct->type, base_addr, field_idx);
            result.value = access_ptr;
        } else if (access->parent->type_info->is_enum_type()) {
            llvm::Constant *constant = llvm_const_int(get_type(access->parent->type_info->base), access->eval.int_val);
            result.value = constant;
        } else if (access->parent->type_info->type_flags & TYPE_FLAG_STRING) {
            LLVM_Addr addr = gen_addr(access->parent);
            if (atoms_match(access->name->name, atom_create(str_lit("data")))) {
                llvm::Value *access_ptr = builder->CreateStructGEP(builtin_string_type, addr.value, 0);
                result.value = access_ptr;
            } else if (atoms_match(access->name->name, atom_create(str_lit("count")))) {
                llvm::Value *access_ptr = builder->CreateStructGEP(builtin_string_type, addr.value, 1);
                result.value = access_ptr;
            }
        } else {
            Assert(0);
        }
        break;
    }

    case AST_SUBSCRIPT:
    {
        Ast_Subscript *subscript = static_cast<Ast_Subscript*>(expr);
        LLVM_Addr addr = gen_addr(subscript->expr);
        LLVM_Value rhs = gen_expr(subscript->index);

        if (subscript->expr->type_info->is_array_type()) {
            llvm::Type *array_type = get_type(subscript->expr->type_info);
            llvm::ArrayRef<llvm::Value*> indices = {
                llvm_zero(llvm::Type::getInt32Ty(*Ctx)), // ptr
                rhs.value  // subscript
            };
            llvm::Value *ptr = builder->CreateGEP(array_type, addr.value, indices);
            result.value = ptr;
        } else if (subscript->expr->type_info->is_pointer_type()) {
            llvm::Type *pointer_type = get_type(subscript->expr->type_info);
            llvm::PointerType* load_type = llvm::PointerType::get(pointer_type, 0);
            llvm::LoadInst *load = builder->CreateLoad(load_type, addr.value);
            llvm::Value *ptr = builder->CreateGEP(pointer_type, load, rhs.value);
            result.value = ptr;
        }
        break;
    }

    case AST_DEREF:
    {
        Ast_Deref *deref = static_cast<Ast_Deref*>(expr);
        LLVM_Addr addr = gen_addr(deref->elem);
        llvm::Type *pointer_type = get_type(deref->elem->type_info);
        llvm::LoadInst *load = builder->CreateLoad(pointer_type, addr.value);
        result.value = load;
        break;
    }
    }

    return result;
}

void LLVM_Backend::get_lazy_expressions(Ast_Binary *root, OP op, Auto_Array<Ast_Expr*> *expr_list) {
    if (root->lhs->is_binop(op)) {
        Ast_Binary *child = static_cast<Ast_Binary*>(root->lhs);
        get_lazy_expressions(child, op, expr_list);
        expr_list->push(root);
    } else {
        expr_list->push(root->lhs);
        expr_list->push(root->rhs);
    }
}

void LLVM_Backend::lazy_eval(Ast_Binary *root, llvm::PHINode *phi_node, llvm::BasicBlock *exit_block) {
    Auto_Array<Ast_Expr*> lazy_hierarchy;
    get_lazy_expressions(root, root->op, &lazy_hierarchy);

    llvm::Value *lazy_constant;
    if (root->op == OP_AND) lazy_constant = llvm::ConstantInt::getFalse(llvm::Type::getInt1Ty(*Ctx));
    else lazy_constant = llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(*Ctx));

    for (Ast_Expr *expr : lazy_hierarchy) {
        bool is_tail = expr == lazy_hierarchy.back();

        llvm::Value *value = NULL;
        if (expr->is_binop(root->op)) {
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
    result.type = get_type(binop->type_info);

    if (binop->expr_flags & EXPR_FLAG_OP_CALL) {
            
    } else {
        if (binop->is_constant()) {
            llvm::Type* type = get_type(binop->type_info);
            if (binop->type_info->is_integral_type()) {
                result.value = llvm::ConstantInt::get(type, binop->eval.int_val, binop->type_info->is_signed());
            } else if (binop->type_info->is_float_type()) {
                result.value = llvm::ConstantFP::get(type, binop->eval.float_val);
            }
        } else {
            if (binop->op == OP_AND || binop->op == OP_OR) {
                llvm::BasicBlock *lazy_exit = llvm_block_new("lazy.exit");
                llvm::PHINode *phi_node = llvm::PHINode::Create(llvm::Type::getInt1Ty(*Ctx), 0);
                lazy_eval(binop, phi_node, lazy_exit);
                llvm::Value *phi_value = builder->Insert(phi_node);
                result.value = phi_value;
            } else {
                LLVM_Value lhs = gen_expr(binop->lhs);
                LLVM_Value rhs = gen_expr(binop->rhs);

                bool is_float = binop->type_info->is_float_type();
                bool is_signed = binop->type_info->is_signed();

                switch (binop->op) {
                default:
                    Assert(0);
                    break;

                case OP_ADD:
                    if (binop->type_info->is_integral_type()) {
                        result.value = builder->CreateAdd(lhs.value, rhs.value);
                    } else if (binop->type_info->is_float_type()) {
                        result.value = builder->CreateFAdd(lhs.value, rhs.value);
                    } else if (binop->type_info->is_pointer_type()) {
                        llvm::Type *type = get_type(binop->type_info->base);
                        if (binop->lhs->type_info->is_pointer_type()) {
                            result.value = builder->CreateGEP(type, lhs.value, rhs.value);
                        } else {
                            result.value = builder->CreateGEP(type, rhs.value, lhs.value);
                        }
                    }
                    break;
                case OP_SUB:
                    result.value = builder->CreateSub(lhs.value, rhs.value);
                    break;
                case OP_MUL:
                    if (is_float) result.value = builder->CreateFMul(lhs.value, rhs.value);
                    else          result.value = builder->CreateMul(lhs.value, rhs.value);
                    break;
                case OP_DIV:
                    result.value = builder->CreateSDiv(lhs.value, rhs.value);
                    break;
                case OP_MOD:
                    result.value = builder->CreateSRem(lhs.value, rhs.value);
                    break;
                case OP_LSH:
                    result.value = builder->CreateShl(lhs.value, rhs.value);
                    break;
                case OP_RSH:
                    result.value = builder->CreateLShr(lhs.value, rhs.value);
                    break;
                case OP_BIT_OR:
                    result.value = builder->CreateOr(lhs.value, rhs.value);
                    break;
                case OP_BIT_AND:
                    result.value = builder->CreateAnd(lhs.value, rhs.value);
                    break;
                case OP_NEQ:
                    result.value = builder->CreateICmp(llvm::CmpInst::ICMP_NE, lhs.value, rhs.value);
                    break;
                case OP_EQ:
                    result.value = builder->CreateICmp(llvm::CmpInst::ICMP_EQ, lhs.value, rhs.value);
                    break;
                case OP_LT:
                    result.value = builder->CreateICmp(llvm::CmpInst::ICMP_SLT, lhs.value, rhs.value);
                    break;
                case OP_GT:
                    result.value = builder->CreateICmp(llvm::CmpInst::ICMP_SGT, lhs.value, rhs.value);
                    break;
                case OP_LTEQ:
                    result.value = builder->CreateICmp(llvm::CmpInst::ICMP_SLE, lhs.value, rhs.value);
                    break;
                case OP_GTEQ:
                    result.value = builder->CreateICmp(llvm::CmpInst::ICMP_SGE, lhs.value, rhs.value);
                    break;
                }
            }
        }
    }
    return result;
}

LLVM_Value LLVM_Backend::gen_null(Ast_Null *null) {
    llvm::Type* type = get_type(null->type_info);
    LLVM_Value value;
    value.value = llvm::Constant::getNullValue(type);
    value.type = type;
    return value;
}

LLVM_Value LLVM_Backend::gen_literal(Ast_Literal *literal) {
    LLVM_Value value = {};

    llvm::Type* type = get_type(literal->type_info);

    bool sign_extend = true;

    switch (literal->literal_flags) {
    default:
        Assert(0);
    case LITERAL_U8:
    case LITERAL_U16:
    case LITERAL_U32:
    case LITERAL_U64:
    case LITERAL_BOOLEAN:
        sign_extend = false;
    case LITERAL_INT:
    case LITERAL_I8:
    case LITERAL_I16:
    case LITERAL_I32:
    case LITERAL_I64:
        value.value = llvm::ConstantInt::get(type, literal->int_val, sign_extend);
        break;
    case LITERAL_FLOAT:
    case LITERAL_F32:
    case LITERAL_F64:
        value.value = llvm::ConstantFP::get(type, literal->float_val);
        break;

    case LITERAL_STRING:
    {
        llvm::Constant *constant_string = llvm::ConstantDataArray::getString(*Ctx, (char *)literal->str_val.data, false);
        type = llvm::ArrayType::get(llvm::Type::getInt8Ty(*Ctx), literal->str_val.count);
        llvm::Value *var = builder->CreateGlobalString((char *)literal->str_val.data);
        value.value = var;
        // llvm::Value* var = LLVMAddGlobal(module, type, ".str");
        // // LLVMSetLinkage(var, LLVMPrivateLinkage);
        // Auto_Array<llvm::Value*> indices;
        // indices.push(LLVMConstInt(LLVMInt32Type(), 0, 0));
        // indices.push(LLVMConstInt(LLVMInt32Type(), 0, 0));
        // value.value = LLVMBuildGEP2(builder, type, var, indices.data, (unsigned)indices.count, ".gep_str");
        // value.type = type;
        break;
    }
    }

    value.type = type; 
    return value;
}

LLVM_Value LLVM_Backend::gen_assignment(Ast_Assignment *assignment) {
    llvm::Type *dest_type = get_type(assignment->type_info);

    LLVM_Addr addr = gen_addr(assignment->lhs);

    LLVM_Value lhs = {};
    if (assignment->op != OP_ASSIGN) {
        // Value of this assignment's identifier
        lhs = gen_expr(assignment->lhs);
    }

    LLVM_Value rhs = gen_expr(assignment->rhs);

    //@Todo Cast between operands??
    switch (assignment->op) {
    case OP_ASSIGN:
        break;
    case OP_ADD_ASSIGN:
        rhs.value = builder->CreateAdd(lhs.value, rhs.value);
        break;
    case OP_SUB_ASSIGN:
        rhs.value = builder->CreateSub(lhs.value, rhs.value);
        break;
    case OP_MUL_ASSIGN:
        rhs.value = builder->CreateMul(lhs.value, rhs.value);
        break;
    case OP_DIV_ASSIGN:
        rhs.value = builder->CreateSDiv(lhs.value, rhs.value);
        break;
    case OP_MOD_ASSIGN:
        rhs.value = builder->CreateSRem(lhs.value, rhs.value);
        break;
    case OP_XOR_ASSIGN:
        rhs.value = builder->CreateXor(lhs.value, rhs.value);
        break;
    case OP_OR_ASSIGN:
        rhs.value = builder->CreateOr(lhs.value, rhs.value);
        break;
    case OP_AND_ASSIGN:
        rhs.value = builder->CreateAnd(lhs.value, rhs.value);
        break;
    case OP_LSH_ASSIGN:
        rhs.value = builder->CreateShl(lhs.value, rhs.value);
        break;
    case OP_RSH_ASSIGN:
        rhs.value = builder->CreateLShr(lhs.value, rhs.value);
        break;
    }

    llvm::Value *value = rhs.value;

    if (rhs.type->isIntegerTy()) {
        value = builder->CreateZExtOrTrunc(value, dest_type);
    } else if (rhs.type->isArrayTy()) {
        // //@Todo @Crash String Literals
        // llvm::ArrayRef<llvm::Value*> indices = {
        //     llvm_zero(llvm::Type::getInt32Ty(*Ctx)),
        //     llvm_zero(llvm::Type::getInt32Ty(*Ctx))
        // };
        // // llvm::Value *base_ptr = builder->CreatePointerCast(rhs.value, dest_type);
        // llvm::Value *ptr = builder->CreateGEP(rhs.type, rhs.value, indices);
        // value = ptr;
    }

    builder->CreateStore(value, addr.value);

    LLVM_Value result;
    result.value = value;
    result.type = dest_type;

    // if (assignment->lhs->type_info->is_struct_type()) {
    //     LLVM_Addr addr = gen_addr(assignment->lhs);
    //     Ast_Type_Info *struct_type = assignment->lhs->type_info;
    //     LLVM_Struct *llvm_struct = lookup_struct(struct_type->decl->name);
    //     if (assignment->rhs->kind == AST_COMPOUND_LITERAL) {
    //         Ast_Compound_Literal *compound = static_cast<Ast_Compound_Literal*>(assignment->rhs);
    //         for (int i = 0; i < compound->elements.count; i++) {
    //             Ast_Expr *elem = compound->elements[i];
    //             LLVM_Value value = gen_expr(elem);
    //             llvm::Value* field_addr = LLVMBuildStructGEP2(builder, llvm_struct->type, addr.value, (unsigned)i, "fieldaddr_tmp");
    //             LLVMBuildStore(builder, value.value, field_addr);
    //         }
    //     } else {
    //         Assert(0);
    //         //@Todo Get addresses of each field in rhs and store in lhs fields
    //     }
    // }

    return result;
}

LLVM_Value LLVM_Backend::gen_expr(Ast_Expr *expr) {
    if (!expr) return {};

    switch (expr->kind) {
    default:
        Assert(0);
        break;

    case AST_NULL:
    {
        Ast_Null *null = static_cast<Ast_Null*>(expr);
        return gen_null(null);
    }
    
    case AST_PAREN:
    {
        Ast_Paren *paren = static_cast<Ast_Paren*>(expr);
        return gen_expr(paren->elem);
    }

    case AST_LITERAL:
    {
        Ast_Literal *literal = static_cast<Ast_Literal*>(expr);
        return gen_literal(literal);
    }

    case AST_COMPOUND_LITERAL:
    {
        Ast_Compound_Literal *literal = static_cast<Ast_Compound_Literal*>(expr);
        break;
    }

    case AST_IDENT:
    {
        LLVM_Value value;
        Ast_Ident *ident = static_cast<Ast_Ident*>(expr);
        Assert(ident->decl);
        LLVM_Var *var = (LLVM_Var *)ident->decl->backend_var;
        Assert(var);
        if (ident->type_info->is_array_type()) {
            value.value = var->alloca;
        } else {
            llvm::LoadInst *load = builder->CreateLoad(var->type, var->alloca);
            value.value = load;
        }
        value.type = var->type;
        return value;
    }

    case AST_CALL:
    {
        Ast_Call *call = static_cast<Ast_Call*>(expr);
        LLVM_Value value = {};
        //@Todo Get address of this, instead of this
        // LLVM_Addr addr = gen_addr(call->elem);
        if (call->elem->kind == AST_IDENT) {
            Ast_Ident *ident = static_cast<Ast_Ident*>(call->elem);
            LLVM_Procedure *procedure = lookup_proc(ident->name);

            Auto_Array<llvm::Value*> args;
            for (int i = 0; i < call->arguments.count; i++) {
                Ast_Expr *arg = call->arguments[i];
                LLVM_Value arg_value = gen_expr(arg);
                args.push(arg_value.value);
            }

            llvm::CallInst *call = builder->CreateCall(procedure->type, procedure->fn, llvm::ArrayRef(args.data, args.count));
            value.value = call;
            value.type = procedure->return_type;
        } else {
            
        }
        return value;
    }
    
    case AST_SUBSCRIPT:
    {
        Ast_Subscript *subscript = static_cast<Ast_Subscript*>(expr);
        llvm::Type *type = get_type(subscript->type_info);
        LLVM_Addr addr = gen_addr(subscript);
        LLVM_Value rhs = gen_expr(subscript->index);
        llvm::LoadInst *load = builder->CreateLoad(type, addr.value);
        LLVM_Value value;
        value.value = load;
        value.type = type;
        return value;
    }

    case AST_CAST:
    {
        Ast_Cast *cast = static_cast<Ast_Cast*>(expr);
        Ast_Expr *elem = cast->elem;
        LLVM_Value expr_value = gen_expr(cast->elem);
        llvm::Type* dest_type = get_type(cast->type_info);

        LLVM_Value value;
        value.type = dest_type;

        if (cast->type_info->is_float_type() && elem->type_info->is_float_type()) {
            value.value = builder->CreateFPCast(expr_value.value, dest_type);
        } else if (cast->type_info->is_integral_type() && elem->type_info->is_integral_type()) {
            value.value = builder->CreateZExtOrTrunc(expr_value.value, dest_type);
            if (cast->type_info->bytes < elem->type_info->bytes) value.value = builder->CreateTrunc(expr_value.value, dest_type);
            else value.value = builder->CreateIntCast(expr_value.value, dest_type, cast->type_info->is_signed());
        } else if (cast->type_info->is_pointer_type() && elem->type_info->is_pointer_type()) {
            value.value = builder->CreatePointerCast(expr_value.value, dest_type);
        } else {
            // Assert(0);
            value.value = expr_value.value;
        }
        return value;
    }
    
    case AST_UNARY:
    {
        Ast_Unary *unary = static_cast<Ast_Unary*>(expr);

        LLVM_Value value;
        if (unary->expr_flags & EXPR_FLAG_OP_CALL) {
        } else {
            if (unary->is_constant()) {
                llvm::Type* type = get_type(unary->type_info);
                if (unary->type_info->is_integral_type()) {
                    value.value = llvm::ConstantInt::get(type, unary->eval.int_val, unary->type_info->is_signed());
                } else if (unary->type_info->is_float_type()) {
                    value.value = llvm::ConstantFP::get(type, unary->eval.float_val);
                }
            } else {
                LLVM_Value elem_value = gen_expr(unary->elem);
                switch (unary->op) {
                case OP_UNARY_PLUS:
                    value.value = elem_value.value;
                    break;
                case OP_UNARY_MINUS:
                    value.value = builder->CreateNeg(elem_value.value);
                    break;
                case OP_BIT_NOT:
                    value.value = builder->CreateNot(elem_value.value);
                    break;
                case OP_NOT:
                    value.value = gen_logical_not(elem_value.value);
                    break;
                }
            }
        }

        value.type = get_type(unary->type_info);
        return value;
    }

    case AST_ADDRESS:
    {
        Ast_Address *address = static_cast<Ast_Address*>(expr);
        LLVM_Value value;
        LLVM_Addr addr = gen_addr(address->elem);
        value.value = addr.value;
        value.type = get_type(address->type_info);
        return value;
    }

    case AST_DEREF:
    {
        Ast_Deref *deref = static_cast<Ast_Deref*>(expr);
        LLVM_Value value;
        LLVM_Addr addr = gen_addr(deref);
        llvm::Type* type = get_type(deref->type_info);
        llvm::LoadInst *load = builder->CreateLoad(type, addr.value);
        value.value = load;
        value.type = type;
        return value;
    }

    case AST_BINARY:
    {
        Ast_Binary *binary = static_cast<Ast_Binary*>(expr);
        return gen_binary_op(binary);
    }

    case AST_ASSIGNMENT:
    {
        Ast_Assignment *assignment = static_cast<Ast_Assignment*>(expr);
        return gen_assignment(assignment);
    }

    case AST_ACCESS:
    {
        Ast_Access *access = static_cast<Ast_Access*>(expr);
        llvm::Type *type = get_type(access->type_info);

        LLVM_Value value;

        if (access->type_info->is_enum_type() && !(access->expr_flags & EXPR_FLAG_LVALUE)) {
            value.value = llvm::ConstantInt::get(type, access->eval.int_val);
        } else {
            LLVM_Addr addr = gen_addr(access);
            llvm::LoadInst *load = builder->CreateLoad(type, addr.value);
            value.value = load;
        }
        value.type = type;
        return value;
    }
    }

    return {};
}

void LLVM_Backend::gen_statement_list(Auto_Array<Ast_Stmt*> statement_list) {
    for (Ast_Stmt *stmt : statement_list) {
        gen_stmt(stmt);
    }
}

void LLVM_Backend::gen_block(Ast_Block *block) {
    gen_statement_list(block->statements);
}

llvm::Value* LLVM_Backend::gen_condition(Ast_Expr *expr) {
    LLVM_Value value = gen_expr(expr);

    llvm::Value* cond = nullptr;
    if (expr->type_info->is_boolean_type()) {
        cond = value.value;
    } else {
        cond = builder->CreateICmpNE(value.value, llvm::Constant::getNullValue(value.type));
    }
    llvm::Value *result = builder->CreateTrunc(cond, llvm::Type::getInt1Ty(*Ctx));
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
    Assert(ifcase->cond->type_info->is_integral_type());
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
            llvm::Type *type = get_type(range->type_info);
            u64 min = range->lhs->eval.int_val, max = range->rhs->eval.int_val;
            for (u64 c = min; c <= max; c++) {
                llvm::ConstantInt *case_constant = static_cast<llvm::ConstantInt*>(llvm::ConstantInt::get(type, c, false));
                switch_inst->addCase(case_constant, (llvm::BasicBlock *)label->backend_block);
            }
        } else {
            LLVM_Value case_cond = gen_expr(label->cond);
            llvm::ConstantInt *case_constant = static_cast<llvm::ConstantInt*>(case_cond.value);
            switch_inst->addCase(case_constant, (llvm::BasicBlock *)label->backend_block);
        }
    }

    builder->Insert(switch_inst);
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

            llvm::Value *gte = builder->CreateICmp(llvm::CmpInst::ICMP_SGE, condition.value, lhs.value);
            llvm::Value *lte = builder->CreateICmp(llvm::CmpInst::ICMP_SLE, condition.value, rhs.value);
            compare = builder->CreateAnd(gte, lte);
        } else {
            if (ifcase->cond) {
                LLVM_Value label_cond = gen_expr(label->cond);
                compare = builder->CreateICmp(llvm::CmpInst::ICMP_EQ, condition.value, label_cond.value);
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

        if (prev->block->statements.count == 0) {
            label->backend_block = prev->backend_block;
        } else {
            label->backend_block = llvm_block_new();
        }
    }

    if (ifcase->switch_jumptable) {
        gen_ifcase_switch(ifcase);
    } else {
        gen_ifcase_if_else(ifcase);
    }

    current_block = nullptr;

    llvm::BasicBlock *case_block = nullptr;
    for (Ast_Case_Label *label : ifcase->cases) {
        //@Note Skip default to ensure default is last on if-else
        if (!ifcase->switch_jumptable && label->is_default) {
            continue;
        }

        if (case_block != label->backend_block) {
            case_block = (llvm::BasicBlock *)label->backend_block;
            emit_block(case_block);
        }

        if (label->block->statements.count == 0) continue;

        gen_block(label->block);

        llvm::BasicBlock *next_block = switch_exit;
        gen_branch(next_block);
    }

    if (!ifcase->switch_jumptable && ifcase->default_case) {
        emit_block((llvm::BasicBlock *)ifcase->default_case->backend_block);
        for (Ast_Stmt *stmt : ifcase->default_case->block->statements) {
            gen_stmt(stmt);
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

    LLVM_Var *var = (LLVM_Var *)for_stmt->var->backend_var;
    Ast_Range *range = static_cast<Ast_Range*>(for_stmt->iterator);
    llvm::Type *it_type = get_type(range->lhs->type_info);
    LLVM_Value range_max = gen_expr(range->rhs);

    gen_var(for_stmt->var);

    llvm::BasicBlock *body = llvm_block_new();
    for_stmt->entry_block = llvm_block_new();
    for_stmt->exit_block = llvm_block_new();
    for_stmt->retry_block = llvm_block_new();

    builder->CreateBr((llvm::BasicBlock *)for_stmt->retry_block);

    current_block = nullptr;

    emit_block((llvm::BasicBlock *)for_stmt->entry_block);
    llvm::Value *it_value = builder->CreateLoad(var->type, var->alloca);
    llvm::Value *iterate = builder->CreateAdd(it_value, llvm::ConstantInt::get(it_type, 1, false));
    builder->CreateStore(iterate, var->alloca);
    builder->CreateBr((llvm::BasicBlock *)for_stmt->retry_block);

    current_block = nullptr;

    emit_block((llvm::BasicBlock *)for_stmt->retry_block);
    it_value = builder->CreateLoad(var->type, var->alloca);
    llvm::Value *condition = builder->CreateICmp(llvm::CmpInst::ICMP_SLT, it_value, range_max.value);
    gen_branch_condition(condition, body, (llvm::BasicBlock *)for_stmt->exit_block);

    emit_block(body);
    gen_block(for_stmt->block);

    gen_branch((llvm::BasicBlock *)for_stmt->entry_block);

    emit_block((llvm::BasicBlock *)for_stmt->exit_block);
}

void LLVM_Backend::gen_var(Ast_Var *var_node) {
    LLVM_Var *var = (LLVM_Var *)var_node->backend_var;
    Assert(var);

    // var->name = var_node->name;
    // var->decl = var_node;
    // var->type = get_type(var_node->type_info);
    // var->alloca = builder->CreateAlloca(var->type, 0, nullptr, llvm::Twine(var->name->data));
    // current_proc->named_values.push(var);

    if (!var_node->init) {
        llvm::Constant *null_value = llvm::Constant::getNullValue(var->type);
        builder->CreateStore(null_value, var->alloca);
    }

    if (var_node->init) {
        Ast_Expr *init = var_node->init;
        if (init->kind == AST_COMPOUND_LITERAL) {
            Ast_Compound_Literal *literal = static_cast<Ast_Compound_Literal*>(init);
            if (var_node->type_info->is_struct_type()) {
                LLVM_Struct *llvm_struct = lookup_struct(var_node->type_info->decl->name);
                for (int i = 0; i < literal->elements.count; i++) {
                    Ast_Expr *elem = literal->elements[i];
                    LLVM_Value value = gen_expr(elem);
                    llvm::Value* field_addr = builder->CreateStructGEP(llvm_struct->type, var->alloca, (unsigned)i);
                    builder->CreateStore(value.value, field_addr);
                }
            } else if (var_node->type_info->is_array_type()) {
                llvm::ArrayType* array_type = static_cast<llvm::ArrayType*>(get_type(var_node->type_info));
                llvm::Type* element_type = array_type->getElementType();

                //@Todo Have to infer if array is filled with constants to just use llvm::ConstantDataArray
                int i = 0;
                for (Ast_Expr *elem : literal->elements) {
                    LLVM_Value value = gen_expr(elem);

                    llvm::Value *idx = llvm::ConstantInt::get(llvm::IntegerType::get(*Ctx, 32), (u64)i);
                    llvm::ArrayRef<llvm::Value*> indices = {
                        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Ctx), 0),
                        idx
                    };
                    llvm::Value *ptr = builder->CreateGEP(array_type, var->alloca, indices);
                    builder->CreateStore(value.value, ptr);
                    i++;
                }
            }
        } else if (init->kind == AST_RANGE) {
            Ast_Range *range = static_cast<Ast_Range*>(init);
            LLVM_Value init_value = gen_expr(range->lhs);
            builder->CreateStore(init_value.value, var->alloca);
        } else if (init->type_info->is_array_type()) {
            LLVM_Value value = gen_expr(init);
            llvm::ArrayRef<llvm::Value*> indices = {
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Ctx), 0),
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Ctx), 0)
            };
            llvm::Value *ptr = builder->CreateGEP(value.type, value.value, indices);
            builder->CreateStore(ptr, var->alloca);
        } else {
            LLVM_Value init_value = gen_expr(init);
            builder->CreateStore(init_value.value, var->alloca);
        }
    }
}

void LLVM_Backend::llvm_store(llvm::Value *value, llvm::Value *address) {
    builder->CreateStore(value, address);
}

void LLVM_Backend::emit_jump(llvm::BasicBlock *target) {
    gen_branch(target);
    llvm::BasicBlock *next_block = llvm_block_new("unreachable");
    emit_block(next_block);
}

void LLVM_Backend::gen_return(Ast_Return *return_stmt) {
    if (return_stmt->expr) {
        LLVM_Value value = gen_expr(return_stmt->expr);
        llvm_store(value.value, current_proc->return_value);
    }
    emit_jump(current_proc->exit_block);
}

void LLVM_Backend::gen_break(Ast_Break *break_stmt) {
    Ast *ast = break_stmt->target;
    Assert(ast);
    llvm::BasicBlock *next_block = nullptr;
    switch (ast->kind) {
    case AST_WHILE:
    {
        Ast_While *while_stmt = static_cast<Ast_While*>(ast);
        next_block = (llvm::BasicBlock *)while_stmt->exit_block;
        break;
    }
    case AST_FOR:
    {
        Ast_For *for_stmt = static_cast<Ast_For*>(ast);
        next_block = (llvm::BasicBlock *)for_stmt->exit_block;
        break;
    }
    case AST_IFCASE:
    {
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
    case AST_WHILE:
    {
        Ast_While *while_stmt = static_cast<Ast_While*>(ast);
        next_block = (llvm::BasicBlock *)while_stmt->entry_block;
        break;
    }
    case AST_FOR: 
    {
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

void LLVM_Backend::gen_stmt(Ast_Stmt *stmt) {
    switch (stmt->kind) {
    case AST_EXPR_STMT:
    {
        Ast_Expr_Stmt *expr_stmt = static_cast<Ast_Expr_Stmt*>(stmt);
        Ast_Expr *expr = expr_stmt->expr;
        LLVM_Value value = gen_expr(expr);
        break;
    }

    case AST_DECL_STMT:
    {
        Ast_Decl_Stmt *decl_stmt = static_cast<Ast_Decl_Stmt*>(stmt);
        Ast_Decl *decl = decl_stmt->decl;
        if (decl->kind == AST_VAR) {
            Ast_Var *var_node = static_cast<Ast_Var*>(decl);
            gen_var(var_node);
        }
        break;
    }

    case AST_IF:
    {
        Ast_If *if_stmt = static_cast<Ast_If*>(stmt);
        gen_if(if_stmt);
        break;
    }

    case AST_IFCASE:
    {
        Ast_Ifcase *ifcase = static_cast<Ast_Ifcase*>(stmt);
        gen_ifcase(ifcase);;
        break;
    }

    case AST_WHILE:
    {
        Ast_While *while_stmt = static_cast<Ast_While*>(stmt);
        gen_while(while_stmt);
        break;
    }

    case AST_FOR:
    {
        Ast_For *for_stmt = static_cast<Ast_For*>(stmt);
        gen_for(for_stmt);
        break;
    }

    case AST_BLOCK:
    {
        Ast_Block *block = static_cast<Ast_Block*>(stmt);
        gen_block(block);
        break;
    }

    case AST_BREAK:
    {
        Ast_Break *break_stmt = static_cast<Ast_Break *>(stmt);
        gen_break(break_stmt);
        break;
    }

    case AST_CONTINUE:
    {
        Ast_Continue *continue_stmt = static_cast<Ast_Continue*>(stmt);
        gen_continue(continue_stmt);
        break;
    }

    case AST_RETURN:
    {
        Ast_Return *return_stmt = static_cast<Ast_Return*>(stmt);
        gen_return(return_stmt);
        break;
    }

    case AST_FALLTHROUGH:
    {
        Ast_Fallthrough *fallthrough = static_cast<Ast_Fallthrough*>(stmt);
        gen_fallthrough(fallthrough);
        break;
    }
    }
}

void LLVM_Backend::gen_decl(Ast_Decl *decl) {
    switch (decl->kind) {
    case AST_PROC:
    {
        Ast_Proc *proc = static_cast<Ast_Proc*>(decl);
        LLVM_Procedure *llvm_proc = gen_procedure(proc);
        global_procedures.push(llvm_proc);
        break;
    }
    case AST_STRUCT:
    {
        Ast_Struct *struct_decl = static_cast<Ast_Struct*>(decl);
        LLVM_Struct *llvm_struct = gen_struct(struct_decl);
        global_structs.push(llvm_struct);
        break;
    }
    }
}

void LLVM_Backend::gen() {
    Ctx = new llvm::LLVMContext();
    Module = new llvm::Module("Main", *Ctx);


    for (int decl_idx = 0; decl_idx < ast_root->declarations.count; decl_idx++) {
        Ast_Decl *decl = ast_root->declarations[decl_idx];
        gen_decl(decl);
    }

    for (int proc_idx = 0; proc_idx < global_procedures.count; proc_idx++) {
        LLVM_Procedure *procedure = global_procedures[proc_idx];
        gen_procedure_body(procedure);
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

    char *linker_args = "msvcrt.lib legacy_stdio_definitions.lib";
    char *linker_command = cstring_fmt("link.exe %s %s", (char *)object_file_name.data, linker_args);

    system(linker_command);
}
