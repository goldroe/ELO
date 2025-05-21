global Arena *llvm_arena;

global LLVM_Procedure *builtin_count__string;
global LLVM_Procedure *builtin_data__string;
global LLVM_Procedure *builtin_init__string;

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

llvm::Value *LLVM_Backend::get_ptr_from_struct_ptr(Ast_Field *field, LLVM_Addr addr) {
    Ast_Field *parent = field->field_parent;
    Assert(parent->type_info->is_pointer_type());
    Ast_Type_Info *struct_type = parent->type_info->base;
    Ast_Ident *name = static_cast<Ast_Ident*>(field->elem);

    Ast_Struct *struct_node = static_cast<Ast_Struct*>(struct_type->decl);
    LLVM_Struct *llvm_struct = lookup_struct(struct_node->name);
    Struct_Field_Info *struct_field = struct_lookup(struct_type, name->name);
    unsigned field_idx = llvm_get_struct_field_index(struct_node, name->name);

    llvm::Type *ptr_type = get_type(parent->type_info);
    llvm::Value *struct_ptr = builder->CreateGEP(ptr_type, addr.value, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Ctx), 0));
    llvm::Value* field_ptr = builder->CreateStructGEP(llvm_struct->type, struct_ptr, field_idx);
    return field_ptr;
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
        llvm::BasicBlock *entry = llvm::BasicBlock::Create(*Ctx, "entry");
        llvm_proc->builder = new llvm::IRBuilder<>(*Ctx);
        llvm_proc->entry = entry;
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

    insert_block(procedure->entry);

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
        llvm::StoreInst *store_inst = builder->CreateStore(argument, var->alloca);
    }

    gen_block(proc->block);

    if (procedure->return_type->isVoidTy()) {
        llvm::BasicBlock *last_block = &procedure->fn->back();
        llvm::Instruction *last_instruction = &last_block->back();

        bool need_return = false;
        if (!last_instruction) {
            need_return = true;
        } else {
            unsigned opcode = last_instruction->getOpcode();
            if (opcode == llvm::Instruction::Ret) {
                need_return = false;
            }
        }

        if (need_return) {
            llvm::ReturnInst *ret = builder->CreateRetVoid();
        }
    }
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

void LLVM_Backend::gen() {
    Ctx = new llvm::LLVMContext();
    Module = new llvm::Module("Main", *Ctx);

    // build string type and operations
    {
        builtin_string_type = llvm::StructType::create(*Ctx, ".builtin.string");
        llvm::ArrayRef<llvm::Type*> elements = {
            llvm::PointerType::get(llvm::Type::getInt8Ty(*Ctx), 0), // data
            llvm::Type::getInt32Ty(*Ctx), // count
        };
        builtin_string_type->setBody(elements, false);

        // builtin.string.init
        {
            Atom *name = atom_create(str8_lit("string_init"));
            llvm::ArrayRef<llvm::Type*> param_types = {
                llvm::PointerType::get(builtin_string_type, 0),
                llvm::PointerType::get(llvm::Type::getInt8Ty(*Ctx), 0)
            };
            llvm::Type *ret_type = llvm::Type::getVoidTy(*Ctx);
            llvm::FunctionType *func_type = llvm::FunctionType::get(ret_type, param_types, false);
            llvm::Function *fn = llvm::Function::Create(func_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, 0, name->data, Module);

            builtin_init__string = llvm_alloc(LLVM_Procedure);
            builtin_init__string->name = name;
            builtin_init__string->proc = NULL;
            builtin_init__string->return_type = ret_type;
            builtin_init__string->type = func_type;
            builtin_init__string->fn = fn;

            llvm::BasicBlock *entry = llvm::BasicBlock::Create(*Ctx, "entry");
            builtin_init__string->builder = new llvm::IRBuilder<>(*Ctx);
            builtin_init__string->entry = entry;
            set_procedure(builtin_init__string);
            insert_block(entry);

            llvm::Argument *string_arg = fn->getArg(0);
            llvm::Argument *cstr = fn->getArg(1);

            llvm::Value *string_ptr   = builder->CreateGEP(param_types[0], string_arg, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Ctx), 0));
            llvm::Value *ptr_to_data  = builder->CreateStructGEP(builtin_string_type, string_ptr, 0);
            llvm::Value *ptr_to_count = builder->CreateStructGEP(builtin_string_type, string_ptr, 1);

            builder->CreateStore(cstr, ptr_to_data);
            builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Ctx), 0), ptr_to_count);

            builder->CreateRetVoid();
        }

        // string.data
        {
            Atom *name = atom_create(str8_lit("builtin.string.data"));
            llvm::Type *param_type = llvm::PointerType::get(builtin_string_type, 0);
            llvm::Type *ret_type   = builtin_string_type->getElementType(0);
            llvm::FunctionType *func_type = llvm::FunctionType::get(ret_type, param_type, false);
            llvm::Function *fn = llvm::Function::Create(func_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, 0, name->data, Module);

            builtin_data__string = llvm_alloc(LLVM_Procedure);
            builtin_data__string->name = name;
            builtin_data__string->proc = NULL;
            builtin_data__string->return_type = ret_type;
            builtin_data__string->type = func_type;
            builtin_data__string->fn = fn;

            llvm::BasicBlock *entry = llvm::BasicBlock::Create(*Ctx, "entry");
            builtin_data__string->builder = new llvm::IRBuilder<>(*Ctx);
            builtin_data__string->entry = entry;
            set_procedure(builtin_data__string);
            insert_block(entry);

            llvm::Argument *argument = fn->getArg(0);

            llvm::Value *string_ptr = builder->CreateGEP(param_type, argument, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Ctx), 0));
            llvm::Value *field_ptr = builder->CreateStructGEP(builtin_string_type, string_ptr, 0);
            llvm::LoadInst *value = builder->CreateLoad(ret_type, field_ptr);
            builder->CreateRet(value);
        }

        // string.count
        {
            Atom *name = atom_create(str8_lit("builtin.string.count"));
            llvm::Type *param_type = llvm::PointerType::get(builtin_string_type, 0);
            llvm::Type *ret_type   = builtin_string_type->getElementType(1);
            llvm::FunctionType *func_type = llvm::FunctionType::get(ret_type, param_type, false);
            llvm::Function *fn = llvm::Function::Create(func_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, 0, name->data, Module);

            builtin_count__string = llvm_alloc(LLVM_Procedure);
            builtin_count__string->name = name;
            builtin_count__string->proc = NULL;
            builtin_count__string->return_type = ret_type;
            builtin_count__string->type = func_type;
            builtin_count__string->fn = fn;

            llvm::BasicBlock *entry = llvm::BasicBlock::Create(*Ctx, "entry");
            builtin_count__string->builder = new llvm::IRBuilder<>(*Ctx);
            builtin_count__string->entry = entry;
            set_procedure(builtin_count__string);
            insert_block(entry);

            llvm::Argument *argument = fn->getArg(0);

            llvm::Value *string_ptr = builder->CreateGEP(param_type, argument, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Ctx), 0));
            llvm::Value *field_ptr = builder->CreateStructGEP(builtin_string_type, string_ptr, 1);
            llvm::LoadInst *value = builder->CreateLoad(ret_type, field_ptr);
            builder->CreateRet(value);
        }
        
    }

    for (int decl_idx = 0; decl_idx < root->declarations.count; decl_idx++) {
        Ast_Decl *decl = root->declarations[decl_idx];
        gen_decl(decl);
    }

    for (int proc_idx = 0; proc_idx < global_procedures.count; proc_idx++) {
        LLVM_Procedure *procedure = global_procedures[proc_idx];
        gen_procedure_body(procedure);
    }

    std::error_code EC;
    llvm::raw_fd_ostream OS("-", EC);
    bool broken_debug_info;
    // Module->print(llvm::errs(), nullptr); // dump IR
    if (llvm::verifyModule(*Module, &OS, &broken_debug_info)) {
        Module->print(llvm::errs(), nullptr); // dump IR
    } else {
        char *gen_file_path = cstring_fmt("%S.bc", path_remove_extension(file->path));
        llvm::raw_fd_ostream FS(gen_file_path, EC);
        llvm::WriteBitcodeToFile(*Module, FS);
    }
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
        case BUILTIN_TYPE_S8:
            return llvm::Type::getInt8Ty(*Ctx);
        case BUILTIN_TYPE_U16:
        case BUILTIN_TYPE_S16:
            return llvm::Type::getInt16Ty(*Ctx);
        case BUILTIN_TYPE_U32:
        case BUILTIN_TYPE_S32:
        case BUILTIN_TYPE_INT:
            return llvm::Type::getInt32Ty(*Ctx);
        case BUILTIN_TYPE_U64:
        case BUILTIN_TYPE_S64:
            return llvm::Type::getInt64Ty(*Ctx);
        case BUILTIN_TYPE_F32:
            return llvm::Type::getFloatTy(*Ctx);
        case BUILTIN_TYPE_F64:
            return llvm::Type::getDoubleTy(*Ctx);
        case BUILTIN_TYPE_STRING:
            return builtin_string_type;
        }
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

    case AST_FIELD:
    {
        Ast_Field *field = static_cast<Ast_Field*>(expr);
        Ast_Field *parent = field->field_parent;
        if (!parent) {
            LLVM_Addr addr = gen_addr(field->elem);
            result.value = addr.value;
        } else {
            LLVM_Addr addr = gen_addr(parent);
            if (parent->type_info->is_struct_type()) {
                Ast_Ident *name = static_cast<Ast_Ident*>(field->elem);
                Ast_Struct *struct_node = static_cast<Ast_Struct*>(parent->type_info->decl);
                LLVM_Struct *llvm_struct = lookup_struct(struct_node->name);
                unsigned idx = llvm_get_struct_field_index(struct_node, name->name);
                llvm::Value* field_ptr_value = builder->CreateStructGEP(llvm_struct->type, addr.value, idx);
                result.value = field_ptr_value;
            } else if (parent->type_info->is_pointer_type()) {
                llvm::Value *field_ptr = get_ptr_from_struct_ptr(field, addr);
                result.value = field_ptr;
            } else if (parent->type_info->type_flags & TYPE_FLAG_STRING) {
                Ast_Ident *name = static_cast<Ast_Ident*>(field->elem);
                if (atoms_match(name->name, atom_create(str_lit("data")))) {
                    llvm::Value *field_ptr = builder->CreateStructGEP(builtin_string_type, addr.value, 0);
                    result.value = field_ptr;
                } else if (atoms_match(name->name, atom_create(str_lit("count")))) {
                    llvm::Value *field_ptr = builder->CreateStructGEP(builtin_string_type, addr.value, 1);
                    result.value = field_ptr;
                }
            }
        }
        break;
    }

    case AST_INDEX:
    {
        Ast_Index *index = static_cast<Ast_Index*>(expr);
        LLVM_Addr addr = gen_addr(index->lhs);
        LLVM_Value rhs = gen_expr(index->rhs);
        llvm::Type *array_type = get_type(index->lhs->type_info);

        llvm::ArrayRef<llvm::Value*> indices = {
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Ctx), 0),  // ptr
            rhs.value  // subscript
        };
        llvm::Value *ptr = builder->CreateGEP(array_type, addr.value, indices);
        result.value = ptr;
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

LLVM_Value LLVM_Backend::gen_binary_op(Ast_Binary *binop) {
    LLVM_Value result = {};
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
            LLVM_Value lhs = gen_expr(binop->lhs);
            LLVM_Value rhs = gen_expr(binop->rhs);

            bool is_float = binop->type_info->is_float_type();
            bool is_signed = binop->type_info->is_signed();

            switch (binop->op.kind) {
            default:
                Assert(0);
                break;

            case TOKEN_PLUS:
                if (binop->type_info->is_integral_type()) {
                    result.value = builder->CreateAdd(lhs.value, rhs.value);
                } else if (binop->type_info->is_float_type()) {
                    result.value = builder->CreateFAdd(lhs.value, rhs.value);
                }
                break;
            case TOKEN_MINUS:
                result.value = builder->CreateSub(lhs.value, rhs.value);
                break;
            case TOKEN_STAR:
                if (is_float) result.value = builder->CreateFMul(lhs.value, rhs.value);
                else          result.value = builder->CreateMul(lhs.value, rhs.value);
                break;
            case TOKEN_SLASH:
                result.value = builder->CreateSDiv(lhs.value, rhs.value);
                break;
            case TOKEN_MOD:
                result.value = builder->CreateSRem(lhs.value, rhs.value);
                break;
            case TOKEN_LSHIFT:
                result.value = builder->CreateShl(lhs.value, rhs.value);
                break;
            case TOKEN_RSHIFT:
                result.value = builder->CreateLShr(lhs.value, rhs.value);
                break;
            case TOKEN_BAR:
                result.value = builder->CreateOr(lhs.value, rhs.value);
                break;
            case TOKEN_AMPER:
                result.value = builder->CreateAnd(lhs.value, rhs.value);
                break;
            case TOKEN_AND:
                Assert(0); // unsupported
                break;
            case TOKEN_OR:
                Assert(0); // unsupported
                break;
            case TOKEN_NEQ:
                result.value = builder->CreateICmp(llvm::CmpInst::ICMP_NE, lhs.value, rhs.value);
                break;
            case TOKEN_EQ2:
                result.value = builder->CreateICmp(llvm::CmpInst::ICMP_EQ, lhs.value, rhs.value);
                break;
            case TOKEN_LT:
                result.value = builder->CreateICmp(llvm::CmpInst::ICMP_SLT, lhs.value, rhs.value);
                break;
            case TOKEN_GT:
                result.value = builder->CreateICmp(llvm::CmpInst::ICMP_SGT, lhs.value, rhs.value);
                break;
            case TOKEN_LTEQ:
                result.value = builder->CreateICmp(llvm::CmpInst::ICMP_SLE, lhs.value, rhs.value);
                break;
            case TOKEN_GTEQ:
                result.value = builder->CreateICmp(llvm::CmpInst::ICMP_SGE, lhs.value, rhs.value);
                break;
            }
        }
    }
    result.type = get_type(binop->type_info);
    return result;
}

LLVM_Value LLVM_Backend::gen_expr(Ast_Expr *expr) {
    LLVM_Value result = {};
    if (!expr) return result;

    switch (expr->kind) {
    case AST_NULL:
    {
        Ast_Null *null = static_cast<Ast_Null*>(expr);
        llvm::Type* type = get_type(expr->type_info);
        result.value = llvm::Constant::getNullValue(type);
        result.type = type;
        break;
    }
    
    case AST_PAREN:
    {
        Ast_Paren *paren = static_cast<Ast_Paren*>(expr);
        result = gen_expr(paren->elem);
        break;
    }

    case AST_LITERAL:
    {
        Ast_Literal *literal = static_cast<Ast_Literal*>(expr);
        llvm::Type* type = get_type(literal->type_info);

        bool sign_extend = true;

        switch (literal->literal_flags) {
        case LITERAL_U8:
        case LITERAL_U16:
        case LITERAL_U32:
        case LITERAL_U64:
        case LITERAL_BOOLEAN:
            sign_extend = false;
        case LITERAL_INT:
        case LITERAL_S8:
        case LITERAL_S16:
        case LITERAL_S32:
        case LITERAL_S64:
            result.value = llvm::ConstantInt::get(type, literal->int_val, sign_extend);
            break;
        case LITERAL_FLOAT:
        case LITERAL_F32:
        case LITERAL_F64:
            result.value = llvm::ConstantFP::get(type, literal->float_val);
            break;

        case LITERAL_STRING:
        {
            llvm::Constant *constant_string = llvm::ConstantDataArray::getString(*Ctx, (char *)literal->str_val.data, false);
            type = llvm::ArrayType::get(llvm::Type::getInt8Ty(*Ctx), literal->str_val.count);
            llvm::Value *var = builder->CreateGlobalString((char *)literal->str_val.data);
            result.value = var;
            // llvm::Value* var = LLVMAddGlobal(module, type, ".str");
            // // LLVMSetLinkage(var, LLVMPrivateLinkage);
            // Auto_Array<llvm::Value*> indices;
            // indices.push(LLVMConstInt(LLVMInt32Type(), 0, 0));
            // indices.push(LLVMConstInt(LLVMInt32Type(), 0, 0));
            // result.value = LLVMBuildGEP2(builder, type, var, indices.data, (unsigned)indices.count, ".gep_str");
            // result.type = type;
            break;
        }
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
        Assert(ident->decl);
        LLVM_Var *var = (LLVM_Var *)ident->decl->backend_var;
        Assert(var);

        llvm::LoadInst *load = builder->CreateLoad(var->type, var->alloca);
        result.value = load;
        result.type = get_type(ident->type_info);
        break;
    }

    case AST_CALL:
    {
        Ast_Call *call = static_cast<Ast_Call*>(expr);
        //@Todo Get address of this, instead of this
        // LLVM_Addr addr = gen_addr(call->elem);
        if (call->elem->kind == AST_IDENT) {
            Ast_Ident *ident = static_cast<Ast_Ident*>(call->elem);
            LLVM_Procedure *procedure = lookup_proc(ident->name);

            if (atoms_match(ident->name, atom_create(str8_lit("string_init")))) {
                procedure = builtin_init__string;
            }

            Auto_Array<llvm::Value*> args;
            for (int i = 0; i < call->arguments.count; i++) {
                Ast_Expr *arg = call->arguments[i];
                LLVM_Value arg_value = gen_expr(arg);
                args.push(arg_value.value);
            }

            llvm::CallInst *call = builder->CreateCall(procedure->type, procedure->fn, llvm::ArrayRef(args.data, args.count));
            result.value = call;
            result.type = procedure->return_type;
        } else {
            
        }
        break;
    }
    
    case AST_INDEX:
    {
        Ast_Index *index = static_cast<Ast_Index*>(expr);
        llvm::ArrayType* array_type = static_cast<llvm::ArrayType*>(get_type(index->lhs->type_info));
        llvm::Type* type = array_type->getElementType();
        LLVM_Addr addr = gen_addr(index);
        LLVM_Value rhs = gen_expr(index->rhs);
        llvm::LoadInst *load = builder->CreateLoad(type, addr.value);
        result.value = load;
        result.type = type;
        break;
    }

    case AST_CAST:
    {
        Ast_Cast *cast = static_cast<Ast_Cast*>(expr);
        Ast_Expr *elem = cast->elem;
        LLVM_Value expr_value = gen_expr(cast->elem);
        llvm::Type* dest_type = get_type(cast->type_info);

        if (cast->type_info->is_float_type() && elem->type_info->is_float_type()) {
            result.value = builder->CreateFPCast(expr_value.value, dest_type);
        } else if (cast->type_info->is_integral_type() && elem->type_info->is_integral_type()) {
            if (cast->type_info->bytes < elem->type_info->bytes) result.value = builder->CreateTrunc(expr_value.value, dest_type);
            else result.value = builder->CreateIntCast(expr_value.value, dest_type, cast->type_info->is_signed());
        }
        break;
    }
    
    case AST_UNARY:
    {
        Ast_Unary *unary = static_cast<Ast_Unary*>(expr);

        if (unary->expr_flags & EXPR_FLAG_OP_CALL) {
            
        } else {
            if (unary->is_constant()) {
                llvm::Type* type = get_type(unary->type_info);
                if (unary->type_info->is_integral_type()) {
                    result.value = llvm::ConstantInt::get(type, unary->eval.int_val, unary->type_info->is_signed());
                } else if (unary->type_info->is_float_type()) {
                    result.value = llvm::ConstantFP::get(type, unary->eval.float_val);
                }
            } else {
                LLVM_Value elem_value = gen_expr(unary->elem);
                switch (unary->op.kind) {
                case TOKEN_MINUS:
                    result.value = builder->CreateNeg(elem_value.value);
                    break;
                case TOKEN_BANG:
                    result.value = builder->CreateNot(elem_value.value);
                    break;
                }
            }
        }

        result.type = get_type(unary->type_info);
        break;
    }

    case AST_ADDRESS:
    {
        Ast_Address *address = static_cast<Ast_Address*>(expr);
        LLVM_Addr addr = gen_addr(address->elem);
        result.value = addr.value;
        result.type = get_type(address->type_info);
        break;
    }

    case AST_DEREF:
    {
        Ast_Deref *deref = static_cast<Ast_Deref*>(expr);
        LLVM_Addr addr = gen_addr(deref);
        llvm::Type* type = get_type(deref->type_info);
        llvm::LoadInst *load = builder->CreateLoad(type, addr.value);
        result.value = load;
        result.type = type;
        break;
    }

    case AST_BINARY:
    {
        Ast_Binary *binary = static_cast<Ast_Binary*>(expr);
        result = gen_binary_op(binary);
        break;
    }

    case AST_ASSIGNMENT:
    {
        Ast_Assignment *assignment = static_cast<Ast_Assignment*>(expr);

        LLVM_Addr addr = gen_addr(assignment->lhs);

        LLVM_Value lhs = {};
        if (assignment->op.kind != TOKEN_EQ) {
            // Value of this assignment's identifier
            lhs = gen_expr(assignment->lhs);
        }

        LLVM_Value rhs = gen_expr(assignment->rhs);
        if (assignment->rhs->kind == AST_NULL) {
        }

        switch (assignment->op.kind) {
        case TOKEN_EQ:
            break;
        case TOKEN_PLUS_EQ:
            rhs.value = builder->CreateAdd(lhs.value, rhs.value);
            break;
        case TOKEN_MINUS_EQ:
            rhs.value = builder->CreateSub(lhs.value, rhs.value);
            break;
        case TOKEN_STAR_EQ:
            rhs.value = builder->CreateMul(lhs.value, rhs.value);
            break;
        case TOKEN_SLASH_EQ:
            rhs.value = builder->CreateSDiv(lhs.value, rhs.value);
            break;
        case TOKEN_MOD_EQ:
            rhs.value = builder->CreateSRem(lhs.value, rhs.value);
            break;
        case TOKEN_XOR_EQ:
            rhs.value = builder->CreateXor(lhs.value, rhs.value);
            break;
        case TOKEN_BAR_EQ:
            rhs.value = builder->CreateOr(lhs.value, rhs.value);
            break;
        case TOKEN_AMPER_EQ:
            rhs.value = builder->CreateAnd(lhs.value, rhs.value);
            break;
        case TOKEN_LSHIFT_EQ:
            rhs.value = builder->CreateShl(lhs.value, rhs.value);
            break;
        case TOKEN_RSHIFT_EQ:
            rhs.value = builder->CreateLShr(lhs.value, rhs.value);
            break;
        }

        builder->CreateStore(rhs.value, addr.value);

        result.value = rhs.value;

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
        break;
    }

    case AST_FIELD:
    {
        Ast_Field *field = static_cast<Ast_Field*>(expr);

        Ast_Field *parent = field->field_parent;
        Ast_Field *child = field->field_child;

        llvm::Type* type = nullptr; 
        llvm::Value *value = nullptr;

        if (parent->type_info->is_struct_type() || parent->type_info->is_pointer_type()) {
            LLVM_Addr addr = gen_addr(field);

            Ast_Type_Info *struct_type = parent->type_info;
            if (struct_type->is_pointer_type()) {
                struct_type = struct_type->base;
            }

            Ast_Ident *name = static_cast<Ast_Ident*>(field->elem);
            Ast_Struct *struct_node = static_cast<Ast_Struct*>(struct_type->decl);
            LLVM_Struct *llvm_struct = lookup_struct(struct_node->name);
            unsigned idx = llvm_get_struct_field_index(struct_node, name->name);

            type = get_type(field->type_info);
            llvm::Value *field_ptr = addr.value;
            llvm::LoadInst* load = builder->CreateLoad(type, field_ptr);
            value = load;
        } else if (parent->type_info->type_flags & TYPE_FLAG_STRING) {
            LLVM_Addr addr = gen_addr(field);
            Ast_Ident *name = static_cast<Ast_Ident*>(field->elem);
            if (atoms_match(name->name, atom_create(str_lit("data")))) {
                type = builtin_string_type->getElementType(0);
                llvm::LoadInst *load = builder->CreateLoad(type, addr.value);
                value = load;
            } else if (atoms_match(name->name, atom_create(str_lit("count")))) {
                type = builtin_string_type->getElementType(1);
                llvm::LoadInst *load = builder->CreateLoad(type, addr.value);
                value = load;
            }
        }
        result.value = value;
        result.type = type;
        break;
    }

    case AST_RANGE:
    {
        break;
    }
    }
    return result;
}

void LLVM_Backend::gen_block(Ast_Block *block) {
    for (int i = 0; i < block->statements.count; i++) {
        Ast_Stmt *stmt = block->statements[i];
        gen_stmt(stmt);
    }
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

    llvm::BasicBlock *exit_block = llvm::BasicBlock::Create(*Ctx, "if.exit");

    for (Ast_If *current = if_stmt; current; current = current->if_next) {
        Ast_If *else_stmt = current->if_next;

        llvm::BasicBlock *then_block = llvm::BasicBlock::Create(*Ctx, "if.then");
        llvm::BasicBlock *else_block = exit_block;

        if (else_stmt) {
            else_block = llvm::BasicBlock::Create(*Ctx, "if.else");
        }

        if (current->is_else) {
            llvm::Value *branch = builder->CreateBr(then_block);
        } else {
            llvm::Value *cond = gen_condition(current->cond);
            llvm::Value *branch = builder->CreateCondBr(cond, then_block, else_block);
        }

        insert_block(then_block);
        gen_block(current->block);
        builder->CreateBr(exit_block);

        insert_block(else_block);
    }
}

void LLVM_Backend::gen_ifcase(Ast_Ifcase *ifcase) {
    bool use_jumptable = ifcase->is_constant && ifcase->cond->type_info->is_integral_type();

    struct Case_Unit {
        Case_Unit *next = nullptr;
        llvm::BasicBlock *basic_block = nullptr;
        Auto_Array<Ast_Case_Label*> labels;
        bool is_default = false;
        Ast_Case_Label *default_label = nullptr;
    };

    Case_Unit *root_unit = NULL;
    Case_Unit *default_unit = NULL;

    Case_Unit *current_unit = NULL;
    for (Ast_Case_Label *label = ifcase->cases[0]; label; label = label->next_label) {
        bool new_unit = true;
        if (label->prev_label && (label->prev_label->fallthrough && label->prev_label->statements.count == 0)) {
            new_unit = false;
            if (label->is_default) {
                new_unit = true;
            } else if (label->prev_label->is_default) {
                new_unit = true;
            }
        }

        if (new_unit) {
            Case_Unit *unit = new Case_Unit();
            unit->basic_block = llvm::BasicBlock::Create(*Ctx);
            if (current_unit) {
                current_unit->next = unit;
            } else {
                root_unit = unit;
            }
            current_unit = unit;
        }

        current_unit->labels.push(label);
        label->backend_block = (void *)current_unit->basic_block;

        if (label->is_default) {
            default_unit = current_unit;
            current_unit->is_default = true;
            current_unit->default_label = label;
        }
    }

    LLVM_Value condition = gen_expr(ifcase->cond);

    if (use_jumptable) {
        llvm::BasicBlock *switch_exit = llvm::BasicBlock::Create(*Ctx, "switch.exit");
        llvm::BasicBlock *default_block = switch_exit;
        if (ifcase->default_case) {
            Assert(default_unit);
            default_block = default_unit->basic_block;
        }

        u64 cases_count = ifcase->cases.count;
        llvm::SwitchInst *switch_inst = llvm::SwitchInst::Create(condition.value, default_block, (unsigned)cases_count);

        for (Case_Unit *unit = root_unit; unit; unit = unit->next) {
            if (unit->is_default) continue;
            for (Ast_Case_Label *label : unit->labels) {
                if (label->cond->kind == AST_RANGE) {
                    Ast_Range *range = static_cast<Ast_Range*>(label->cond);
                    llvm::Type *type = get_type(range->type_info);
                    u64 r0 = 0, r1 = 0;
                    if (range->lhs->kind == AST_LITERAL) {
                        r0 = static_cast<Ast_Literal*>(range->lhs)->int_val;
                    }
                    if (range->rhs->kind == AST_LITERAL) {
                        r1 = static_cast<Ast_Literal*>(range->rhs)->int_val;
                    }
                    for (u64 p = r0; p <= r1; p++) {
                        llvm::ConstantInt *case_constant = static_cast<llvm::ConstantInt*>(llvm::ConstantInt::get(type, p, false));
                        switch_inst->addCase(case_constant, unit->basic_block);
                    }
                } else {
                    LLVM_Value case_cond = gen_expr(label->cond);
                    llvm::ConstantInt *case_constant = static_cast<llvm::ConstantInt*>(case_cond.value);
                    switch_inst->addCase(case_constant, unit->basic_block);
                }
            }
        }

        builder->Insert(switch_inst);

        for (Case_Unit *unit = root_unit; unit; unit = unit->next) {
            if (unit->is_default) continue;

            llvm::BasicBlock *basic_block = unit->basic_block;
            insert_block(basic_block);

            for (Ast_Case_Label *label : unit->labels) {
                for (Ast_Stmt *stmt : label->statements) {
                    gen_stmt(stmt);
                }
            }

            Ast_Case_Label *last_label = unit->labels.back();
            llvm::BasicBlock *jump = switch_exit;
            if (last_label->fallthrough && unit->next) {
                jump = unit->next->basic_block;
            }
            llvm::Value *branch = builder->CreateBr(jump);
        }

        if (ifcase->default_case) {
            Assert(default_unit);
            insert_block(default_block);
            for (Ast_Stmt *stmt : default_unit->labels[0]->statements) {
                gen_stmt(stmt);
            }

            llvm::BasicBlock *jump = switch_exit;
            if (ifcase->default_case->fallthrough && default_unit->next) {
                jump = default_unit->next->basic_block;
            }
            llvm::Value *branch = builder->CreateBr(jump);
        }

        insert_block(switch_exit);
    } else {
        llvm::BasicBlock *jump_exit = llvm::BasicBlock::Create(*Ctx);
        llvm::BasicBlock *switch_exit = llvm::BasicBlock::Create(*Ctx);

        llvm::BasicBlock *default_block = switch_exit;
        if (ifcase->default_case) {
            default_block = default_unit->basic_block;
        }

        //@Note Artificial jump table
        for (Ast_Case_Label *label : ifcase->cases) {
            // Skip default to ensure it is only checked at the end
            if (label->is_default) continue;
            Ast_Case_Label *else_label = label->next_label;
            if (else_label && else_label->is_default) {
                else_label = else_label->next_label;
            }

            llvm::BasicBlock *else_block = jump_exit;
            if (else_label) {
                else_block = llvm::BasicBlock::Create(*Ctx);
            }

            llvm::Value *compare;
            if (label->cond->kind == AST_RANGE) {
                //@Note Check if condition is within range
                Ast_Range *range = static_cast<Ast_Range*>(label->cond);
                LLVM_Value lhs = gen_expr(range->lhs);
                LLVM_Value rhs = gen_expr(range->rhs);

                llvm::Value *gte = builder->CreateICmp(llvm::CmpInst::ICMP_SGE, condition.value, lhs.value);
                llvm::Value *lte = builder->CreateICmp(llvm::CmpInst::ICMP_SLE, condition.value, rhs.value);
                compare = builder->CreateAnd(gte, lte);
            } else {
                LLVM_Value label_cond = gen_expr(label->cond);
                compare = builder->CreateICmp(llvm::CmpInst::ICMP_EQ, condition.value, label_cond.value);
            }
            builder->CreateCondBr(compare, (llvm::BasicBlock *)label->backend_block, else_block);

            insert_block(else_block);
        }
        //@Note End of jump table
        insert_block(jump_exit);
        builder->CreateBr(default_block);

        //@Note Actual ifcase units
        for (Case_Unit *unit = root_unit; unit; unit = unit->next) {
            if (unit->is_default) continue;

            llvm::BasicBlock *basic_block = unit->basic_block;
            insert_block(basic_block);

            for (Ast_Case_Label *label : unit->labels) {
                for (Ast_Stmt *stmt : label->statements) {
                    gen_stmt(stmt);
                }
            }

            Ast_Case_Label *last_label = unit->labels.back();
            llvm::BasicBlock *jump = switch_exit;
            if (last_label->fallthrough && unit->next) {
                jump = unit->next->basic_block;
            }
            llvm::Value *branch = builder->CreateBr(jump);
        }

        if (ifcase->default_case) {
            insert_block(default_unit->basic_block);
            for (Ast_Stmt *stmt : default_unit->labels[0]->statements) {
                gen_stmt(stmt);
            }

            llvm::BasicBlock *jump = switch_exit;
            if (ifcase->default_case->fallthrough && default_unit->next) {
                jump = default_unit->next->basic_block;
            }
            llvm::Value *branch = builder->CreateBr(jump);
        }

        insert_block(switch_exit);
    }

    for (Case_Unit *unit = root_unit; unit; ) {
        Case_Unit *temp = unit;
        unit = unit->next;
        delete temp;
    }
    
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

    LLVM_Procedure *proc = current_proc;

    llvm::BasicBlock *head = llvm::BasicBlock::Create(*Ctx, "loop_head");
    llvm::BasicBlock *body = llvm::BasicBlock::Create(*Ctx, "loop_body");
    llvm::BasicBlock *tail = llvm::BasicBlock::Create(*Ctx, "loop_tail");

    builder->CreateBr(head);

    // head
    insert_block(head);

    llvm::Value* condition = gen_condition(while_stmt->cond);
    builder->CreateCondBr(condition, body, tail);

    // body
    insert_block(body);
    gen_block(while_stmt->block);
    builder->CreateBr(head);

    // tail
    insert_block(tail);
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

    if (for_stmt->init) gen_stmt(for_stmt->init);

    llvm::BasicBlock *head = llvm::BasicBlock::Create(*Ctx, "loop_head");
    llvm::BasicBlock *body = llvm::BasicBlock::Create(*Ctx, "loop_body");
    llvm::BasicBlock *tail = llvm::BasicBlock::Create(*Ctx, "loop_tail");

    builder->CreateBr(head);

    // head
    insert_block(head);

    if (for_stmt->cond) {
        llvm::Value* condition = gen_condition(for_stmt->cond);
        builder->CreateCondBr(condition, body, tail);
    } else {
        builder->CreateBr(body);
    }

    // body
    insert_block(body);
    gen_block(for_stmt->block);

    gen_expr(for_stmt->iterator);

    builder->CreateBr(head);

    // tail
    insert_block(tail);
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
                for (int i = 0; i < literal->elements.count; i++) {
                    Ast_Expr *elem = literal->elements[i];
                    LLVM_Value value = gen_expr(elem);

                    llvm::Value *idx = llvm::ConstantInt::get(llvm::IntegerType::get(*Ctx, 32), (uint64_t)i);
                    llvm::ArrayRef<llvm::Value*> indices = {
                        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Ctx), 0),
                        idx
                    };
                    llvm::Value *ptr = builder->CreateGEP(array_type, var->alloca, indices);
                    builder->CreateStore(value.value, ptr);
                }
            }
        } else {
            LLVM_Value init_value = gen_expr(init);
            builder->CreateStore(init_value.value, var->alloca);
        }
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

    case AST_RETURN:
    {
        Ast_Return *return_stmt = static_cast<Ast_Return*>(stmt);
        if (return_stmt->expr) {
            LLVM_Value ret = gen_expr(return_stmt->expr);
            builder->CreateRet(ret.value);
        } else {
            builder->CreateRetVoid();
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

void LLVM_Backend::insert_block(llvm::BasicBlock *block) {
    llvm::Function *fn = current_proc->fn;
    fn->insert(fn->end(), block);
    builder->SetInsertPoint(block);
    current_block = block;
}
