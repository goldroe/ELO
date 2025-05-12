global Arena *llvm_arena;
#define llvm_alloc(T) (T*)llvm_backend_alloc(sizeof(T), alignof(T))

internal void *llvm_backend_alloc(u64 size, int alignment) {
    void *result = (void *)arena_alloc(llvm_arena, size, alignment);
    MemoryZero(result, size);
    return result;
}

LLVMBasicBlockRef LLVM_Backend::llvm_block_new(const char *s) {
    LLVMBasicBlockRef result = LLVMCreateBasicBlockInContext(context, s);
    return result;
}

void LLVM_Backend::llvm_emit_block(LLVMBasicBlockRef block) {
    LLVMAppendExistingBasicBlock(current_proc->value, block);
    LLVMPositionBuilderAtEnd(builder, block);
    current_block = block;
}

LLVM_Procedure *LLVM_Backend::printf_proc_emit() {
    LLVM_Procedure *llvm_proc = llvm_alloc(LLVM_Procedure);
    llvm_proc->name = atom_create(str8_lit("printf"));
    llvm_proc->proc = NULL;

    llvm_proc->parameter_types.push(LLVMPointerType(LLVMInt8Type(), 0));
    llvm_proc->return_type = LLVMInt32Type();

    llvm_proc->type = LLVMFunctionType(llvm_proc->return_type, llvm_proc->parameter_types.data, (int)llvm_proc->parameter_types.count, true);

    llvm_proc->value = LLVMAddFunction(module, "printf", llvm_proc->type);

    LLVMLinkage linkage = LLVMGetLinkage(llvm_proc->value);
    LLVMSetLinkage(llvm_proc->value, LLVMExternalLinkage);
    LLVMSetFunctionCallConv(llvm_proc->value, LLVMCCallConv);

    return llvm_proc;
}

void LLVM_Backend::emit() {
    module = LLVMModuleCreateWithName("my_module");
    context = LLVMGetModuleContext(module);

    printf_procedure = printf_proc_emit();

    for (int decl_idx = 0; decl_idx < root->declarations.count; decl_idx++) {
        Ast_Decl *decl = root->declarations[decl_idx];
        emit_decl(decl);
    }

    for (int proc_idx = 0; proc_idx < global_procedures.count; proc_idx++) {
        LLVM_Procedure *procedure = global_procedures[proc_idx];
        emit_procedure_body(procedure);
    }

    char *error = NULL;
    LLVMVerifyModule(module, LLVMPrintMessageAction, &error);
    LLVMDisposeMessage(error);

    char *gen_file_path = cstring_fmt("%S.bc", path_remove_extension(file->path));
    if (LLVMWriteBitcodeToFile(module, gen_file_path) != 0) {
        fprintf(stderr, "error writing bitcode to file %s, skipping\n", gen_file_path);
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

internal LLVM_Var *llvm_get_named_value(LLVM_Procedure *proc, Atom *name) {
    for (int i = 0; i < proc->named_values.count; i++) {
        LLVM_Var *var = proc->named_values[i];
        if (atoms_match(var->name, name)) {
            return var;
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

LLVMTypeRef LLVM_Backend::emit_type(Ast_Type_Info *type_info) {
    LLVMTypeRef result = 0;

    if (type_info->type_flags & TYPE_FLAG_BUILTIN) {
        switch (type_info->builtin_kind) {
        case BUILTIN_TYPE_NULL:
            Assert(0); //@Note No expression should still have the null type, needs to have type of "owner"
            result = LLVMVoidType();
            break;
        case BUILTIN_TYPE_VOID:
            result = LLVMVoidType();
            break;
        case BUILTIN_TYPE_U8:
        case BUILTIN_TYPE_S8:
            result = LLVMInt8Type();
            break;
        case BUILTIN_TYPE_U16:
        case BUILTIN_TYPE_S16:
            result = LLVMInt16Type();
            break;
        case BUILTIN_TYPE_U32:
        case BUILTIN_TYPE_S32:
        case BUILTIN_TYPE_INT:
        case BUILTIN_TYPE_BOOL:
            result = LLVMInt32Type();
            break;
        case BUILTIN_TYPE_U64:
        case BUILTIN_TYPE_S64:
            result = LLVMInt64Type();
            break;
        case BUILTIN_TYPE_F32:
            result = LLVMFloatType();
            break;
        case BUILTIN_TYPE_F64:
            result = LLVMDoubleType();
            break;
        }
    } else if (type_info->is_struct_type()) {
        LLVM_Struct *llvm_struct = lookup_struct(type_info->decl->name);
        result = llvm_struct->type;
    } else if (type_info->is_array_type()) {
        Ast_Array_Type_Info *array_type_info = static_cast<Ast_Array_Type_Info*>(type_info);
        LLVMTypeRef element_type = emit_type(array_type_info->base);
        LLVMTypeRef array_type = LLVMArrayType2(element_type, array_type_info->array_size);
        result = array_type;
    } else if (type_info->is_pointer_type()) {
        LLVMTypeRef element_type = emit_type(type_info->base);
        LLVMTypeRef pointer_type = LLVMPointerType(element_type, 0);
        result = pointer_type;
    }

    Assert(result);

    return result;
}

LLVM_Addr LLVM_Backend::emit_addr(Ast_Expr *expr) {
    LLVM_Addr result = {};

    switch (expr->kind) {
    default:
        Assert(0);
        break;

    case AST_IDENT:
    {
        Ast_Ident *ident = static_cast<Ast_Ident*>(expr);
        LLVM_Var *var = llvm_get_named_value(current_proc, ident->name);
        Assert(var);
        result.value = var->alloca;
        // printf("ident: %s %p, value: %s %p.\n", ident->name->data, ident->name, var->name->data, var->name);
        break;
    }

    case AST_FIELD:
    {
        Ast_Field *field = static_cast<Ast_Field*>(expr);
        Ast_Field *parent = field->field_parent;
        if (!parent) {
            LLVM_Addr addr = emit_addr(field->elem);
            result.value = addr.value;
        } else {
            LLVM_Addr addr = emit_addr(parent);
            if (parent->type_info->is_struct_type()) {
                Ast_Ident *name = static_cast<Ast_Ident*>(field->elem);
                Ast_Struct *struct_node = static_cast<Ast_Struct*>(parent->type_info->decl);
                LLVM_Struct *llvm_struct = lookup_struct(struct_node->name);
                unsigned idx = llvm_get_struct_field_index(struct_node, name->name);
                LLVMValueRef field_ptr_value = LLVMBuildStructGEP2(builder, llvm_struct->type, addr.value, idx, "fieldaddr");
                result.value = field_ptr_value;
            } else if (parent->type_info->is_pointer_type() && parent->type_info->base->is_struct_type()) {
                Ast_Type_Info *struct_type = parent->type_info->base;
                Ast_Ident *name = static_cast<Ast_Ident*>(field->elem);
                Ast_Struct *struct_node = static_cast<Ast_Struct*>(struct_type->decl);
                LLVM_Struct *llvm_struct = lookup_struct(struct_node->name);
                Struct_Field_Info *struct_field = struct_lookup(struct_type, name->name);
                unsigned idx = llvm_get_struct_field_index(struct_node, name->name);

                // Deref pointer
                LLVMTypeRef typeref = emit_type(parent->type_info);
                LLVMValueRef ptr_value = LLVMBuildLoad2(builder, typeref, addr.value, "fieldptr");

                // Get element pointer based on deref 
                LLVMValueRef field_ptr = LLVMBuildStructGEP2(builder, llvm_struct->type, ptr_value, idx, "fieldaddr2");
                result.value = field_ptr;
            }
        }
        break;
    }

    case AST_INDEX:
    {
        Ast_Index *index = static_cast<Ast_Index*>(expr);
        LLVM_Addr addr = emit_addr(index->lhs);
        LLVM_Value rhs = emit_expr(index->rhs);
        LLVMTypeRef type = emit_type(index->type_info);
        LLVMValueRef value = LLVMBuildGEP2(builder, type, addr.value, &rhs.value, 1, "indexaddr");
        result.value = value;
        break;
    }

    case AST_DEREF:
    {
        Ast_Deref *deref = static_cast<Ast_Deref*>(expr);
        LLVM_Addr addr = emit_addr(deref->elem);
        result.value = addr.value;
        break;
    }
    }

    return result;
}

LLVM_Value LLVM_Backend::emit_binary_op(Ast_Binary *binop) {
    LLVM_Value result = {};
    if (binop->expr_flags & EXPR_FLAG_OP_CALL) {
            
    } else {
        if (binop->is_constant()) {
            LLVMTypeRef type = emit_type(binop->type_info);
            if (binop->type_info->is_integral_type()) {
                result.value = LLVMConstInt(type, binop->eval.int_val, binop->type_info->is_signed());
            } else if (binop->type_info->is_float_type()) {
                result.value = LLVMConstReal(type, binop->eval.float_val);
            }
        } else {
            LLVM_Value lhs = emit_expr(binop->lhs);
            LLVM_Value rhs = emit_expr(binop->rhs);

            bool is_float = binop->type_info->is_float_type();
            bool is_signed = binop->type_info->is_signed();

            switch (binop->op.kind) {
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
                result.value = (is_float ? LLVMBuildFMul : LLVMBuildMul)(builder, lhs.value, rhs.value, "multmp");
                break;
            case TOKEN_SLASH:
                result.value = (is_signed ? LLVMBuildSDiv : LLVMBuildUDiv)(builder, lhs.value, rhs.value, "divtmp");
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
            case TOKEN_NEQ:
                result.value = LLVMBuildICmp(builder, LLVMIntNE, lhs.value, rhs.value, "neq");
                break;
            case TOKEN_EQ2:
                result.value = LLVMBuildICmp(builder, LLVMIntEQ, lhs.value, rhs.value, "eq2");
                break;
            case TOKEN_LT:
                result.value = LLVMBuildICmp(builder, is_signed ? LLVMIntSLT : LLVMIntULT, lhs.value, rhs.value, "lttmp");
                break;
            case TOKEN_GT:
                result.value = LLVMBuildICmp(builder, is_signed ? LLVMIntSGT : LLVMIntUGT, lhs.value, rhs.value, "gttmp");
                break;
            case TOKEN_LTEQ:
                result.value = LLVMBuildICmp(builder, is_signed ? LLVMIntSLE : LLVMIntULE, lhs.value, rhs.value, "ltetmp");
                break;
            case TOKEN_GTEQ:
                result.value = LLVMBuildICmp(builder, is_signed ? LLVMIntSGE : LLVMIntUGE, lhs.value, rhs.value, "gtetmp");
                break;
            }
        }
    }
    result.type = emit_type(binop->type_info);
    return result;
}

LLVM_Value LLVM_Backend::emit_expr(Ast_Expr *expr) {
    LLVM_Value result = {};
    if (!expr) return result;

    switch (expr->kind) {
    case AST_NULL:
    {
        Ast_Null *null = static_cast<Ast_Null*>(expr);
        LLVMTypeRef type = emit_type(expr->type_info);
        result.value = LLVMConstNull(type);
        result.type = type;
        break;
    }
    
    case AST_PAREN:
    {
        Ast_Paren *paren = static_cast<Ast_Paren*>(expr);
        result = emit_expr(paren->elem);
        break;
    }

    case AST_LITERAL:
    {
        Ast_Literal *literal = static_cast<Ast_Literal*>(expr);
        LLVMTypeRef type = emit_type(literal->type_info);

        if (literal->literal_flags & LITERAL_INT) {
            result.value = LLVMConstInt(type, literal->int_val, (literal->type_info->type_flags & TYPE_FLAG_SIGNED));
        } else if (literal->literal_flags & LITERAL_FLOAT) {
            result.value = LLVMConstReal(type, literal->float_val);
        } else if (literal->literal_flags & LITERAL_STRING) {
            LLVMValueRef value = LLVMConstString((char *)literal->str_val.data, (unsigned)literal->str_val.count, false);
            type = LLVMArrayType(LLVMInt32Type(), (unsigned)literal->str_val.count);

            // LLVMValueRef var = LLVMAddGlobal(module, type, ".str");
            // // LLVMSetLinkage(var, LLVMPrivateLinkage);
            // Auto_Array<LLVMValueRef> indices;
            // indices.push(LLVMConstInt(LLVMInt32Type(), 0, 0));
            // indices.push(LLVMConstInt(LLVMInt32Type(), 0, 0));
            // result.value = LLVMBuildGEP2(builder, type, var, indices.data, (unsigned)indices.count, ".gep_str");
            // result.type = type;

            LLVMValueRef var = LLVMBuildGlobalString(builder, (char *)literal->str_val.data, ".str");
            result.value = var;
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
        LLVM_Var *var = llvm_get_named_value(current_proc, ident->name);
        Assert(var);

        result.value = LLVMBuildLoad2(builder, var->type, var->alloca, (char *)var->name->data);
        result.type = emit_type(ident->type_info);
        break;
    }

    case AST_CALL:
    {
        Ast_Call *call = static_cast<Ast_Call*>(expr);
        //@Todo Get address of this, instead of this
        // LLVM_Addr addr = emit_addr(call->elem);
        if (call->elem->kind == AST_IDENT) {
            Ast_Ident *ident = static_cast<Ast_Ident*>(call->elem);
            LLVM_Procedure *procedure = lookup_proc(ident->name);

            if (atoms_match(ident->name, atom_create(str8_lit("printf")))) {
                procedure = printf_procedure;
            }

            Auto_Array<LLVMValueRef> args;
            for (int i = 0; i < call->arguments.count; i++) {
                Ast_Expr *arg = call->arguments[i];
                LLVM_Value arg_value = emit_expr(arg);
                args.push(arg_value.value);
            }

            result.value = LLVMBuildCall2(builder, procedure->type, procedure->value, args.data, (unsigned int)args.count, "calltmp");
            result.type = procedure->return_type;
        } else {
            
        }
        break;
    }
    
    case AST_INDEX:
    {
        Ast_Index *index = static_cast<Ast_Index*>(expr);
        LLVMTypeRef array_type = emit_type(index->lhs->type_info);
        LLVMTypeRef type = emit_type(index->type_info);
        LLVM_Addr addr = emit_addr(index->lhs);
        LLVM_Value rhs = emit_expr(index->rhs);
        LLVMValueRef pointer = LLVMBuildGEP2(builder, array_type, addr.value, &rhs.value, 1, "indexaddr");
        LLVMValueRef value = LLVMBuildLoad2(builder, type, pointer, "indextmp");
        result.value = value;
        result.type = type;
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

        if (unary->expr_flags & EXPR_FLAG_OP_CALL) {
            
        } else {
            if (unary->is_constant()) {
                LLVMTypeRef type = emit_type(unary->type_info);
                if (unary->type_info->is_integral_type()) {
                    result.value = LLVMConstInt(type, unary->eval.int_val, unary->type_info->is_signed());
                } else if (unary->type_info->is_float_type()) {
                    result.value = LLVMConstReal(type, unary->eval.float_val);
                }
            } else {
                LLVM_Value elem_value = emit_expr(unary->elem);
                switch (unary->op.kind) {
                case TOKEN_MINUS:
                    result.value = LLVMBuildNeg(builder, elem_value.value, "negtmp");
                    break;
                case TOKEN_BANG:
                    result.value = LLVMBuildNot(builder, elem_value.value, "nottmp");
                    break;
                }
            }
        }

        result.type = emit_type(unary->type_info);
        break;
    }

    case AST_ADDRESS:
    {
        Ast_Address *address = static_cast<Ast_Address*>(expr);
        LLVM_Addr addr = emit_addr(address->elem);
        result.value = addr.value;
        result.type = emit_type(address->type_info);
        break;
    }

    case AST_DEREF:
    {
        Ast_Deref *deref = static_cast<Ast_Deref*>(expr);
        LLVM_Addr addr = emit_addr(deref->elem);
        LLVMTypeRef type = emit_type(deref->type_info);
        LLVMValueRef value = LLVMBuildLoad2(builder, type, addr.value, "dereftmp");
        result.value = value;
        result.type = type;
        printf("%s\n", string_from_type(deref->type_info));
        break;
    }

    case AST_BINARY:
    {
        Ast_Binary *binary = static_cast<Ast_Binary*>(expr);
        result = emit_binary_op(binary);
        break;
    }

    case AST_ASSIGNMENT:
    {
        Ast_Assignment *assignment = static_cast<Ast_Assignment*>(expr);

        LLVM_Addr addr = emit_addr(assignment->lhs);

        LLVM_Value lhs = {};
        if (assignment->op.kind != TOKEN_EQ) {
            // Value of this assignment's identifier
            lhs = emit_expr(assignment->lhs);
        }

        LLVM_Value rhs = emit_expr(assignment->rhs);
        if (assignment->rhs->kind == AST_NULL) {
        }

        switch (assignment->op.kind) {
        case TOKEN_EQ:
            break;
        case TOKEN_PLUS_EQ:
            rhs.value = LLVMBuildAdd(builder, lhs.value, rhs.value, "addeq");
            break;
        case TOKEN_MINUS_EQ:
            rhs.value = LLVMBuildSub(builder, lhs.value, rhs.value, "subeq");
            break;
        case TOKEN_STAR_EQ:
            rhs.value = LLVMBuildMul(builder, lhs.value, rhs.value, "muleq");
            break;
        case TOKEN_SLASH_EQ:
            rhs.value = LLVMBuildSDiv(builder, lhs.value, rhs.value, "diveq");
            break;
        case TOKEN_MOD_EQ:
            rhs.value = LLVMBuildSRem(builder, lhs.value, rhs.value, "modeq");
            break;
        case TOKEN_XOR_EQ:
            rhs.value = LLVMBuildXor(builder, lhs.value, rhs.value, "xoreq");
            break;
        case TOKEN_BAR_EQ:
            rhs.value = LLVMBuildOr(builder, lhs.value, rhs.value, "oreq");
            break;
        case TOKEN_AMPER_EQ:
            rhs.value = LLVMBuildAnd(builder, lhs.value, rhs.value, "andeq");
            break;
        case TOKEN_LSHIFT_EQ:
            rhs.value = LLVMBuildShl(builder, lhs.value, rhs.value, "shleq");
            break;
        case TOKEN_RSHIFT_EQ:
            rhs.value = LLVMBuildAShr(builder, lhs.value, rhs.value, "shreq");
            break;
        }

        LLVMBuildStore(builder, rhs.value, addr.value);

        result.value = rhs.value;

        // if (assignment->lhs->type_info->is_struct_type()) {
        //     LLVM_Addr addr = emit_addr(assignment->lhs);
        //     Ast_Type_Info *struct_type = assignment->lhs->type_info;
        //     LLVM_Struct *llvm_struct = lookup_struct(struct_type->decl->name);
        //     if (assignment->rhs->kind == AST_COMPOUND_LITERAL) {
        //         Ast_Compound_Literal *compound = static_cast<Ast_Compound_Literal*>(assignment->rhs);
        //         for (int i = 0; i < compound->elements.count; i++) {
        //             Ast_Expr *elem = compound->elements[i];
        //             LLVM_Value value = emit_expr(elem);
        //             LLVMValueRef field_addr = LLVMBuildStructGEP2(builder, llvm_struct->type, addr.value, (unsigned)i, "fieldaddr_tmp");
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

        LLVM_Addr addr = emit_addr(field);

        Ast_Type_Info *struct_type = parent->type_info;
        if (struct_type->is_pointer_type()) {
            struct_type = struct_type->base;
        }

        Ast_Ident *name = static_cast<Ast_Ident*>(field->elem);
        Ast_Struct *struct_node = static_cast<Ast_Struct*>(struct_type->decl);
        LLVM_Struct *llvm_struct = lookup_struct(struct_node->name);
        unsigned idx = llvm_get_struct_field_index(struct_node, name->name);

        LLVMTypeRef type = emit_type(field->type_info);

        if (parent->type_info->is_pointer_type()) {
            // Deref pointer
            LLVMTypeRef typeref = emit_type(parent->type_info);
            LLVMValueRef ptr_value = LLVMBuildLoad2(builder, typeref, addr.value, "fieldptr");
            // Get element pointer based on deref
            LLVMValueRef field_ptr = LLVMBuildStructGEP2(builder, llvm_struct->type, ptr_value, idx, "fieldaddr2");
            result.value = field_ptr;
        } else {
            LLVMValueRef field_ptr = LLVMBuildStructGEP2(builder, llvm_struct->type, addr.value, idx, "fieldaddr_tmp");
            LLVMValueRef field_value = LLVMBuildLoad2(builder, type, field_ptr,"fieldload");
            result.value = field_value;
        }
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

void LLVM_Backend::emit_block(Ast_Block *block) {
    for (int i = 0; i < block->statements.count; i++) {
        Ast_Stmt *stmt = block->statements[i];
        emit_stmt(stmt);
    }
}

LLVMValueRef LLVM_Backend::emit_condition(Ast_Expr *expr) {
    LLVM_Value value = emit_expr(expr);

    LLVMValueRef cond = nullptr;
    if (expr->type_info->is_boolean_type()) {
        cond = value.value;
    } else {
        cond = LLVMBuildICmp(builder, LLVMIntNE, value.value, LLVMConstNull(value.type), "ICmp");
    }
    return cond;
}

void LLVM_Backend::emit_if(Ast_If *if_stmt) {
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
    Ast_If *else_stmt = if_stmt->if_next;

    LLVMBasicBlockRef exit_block = llvm_block_new("if_exit");
    LLVMBasicBlockRef then_block = exit_block;
    LLVMBasicBlockRef else_block = exit_block;

    then_block = llvm_block_new("if_then");
    if (else_stmt) {
        else_block = llvm_block_new("if_else");
    }

    LLVMValueRef cond = emit_condition(if_stmt->cond);
    LLVMValueRef branch = LLVMBuildCondBr(builder, cond, then_block, exit_block);

    llvm_emit_block(then_block);
    emit_block(if_stmt->block);
    LLVMBuildBr(builder, exit_block);

    if (else_stmt) {
        llvm_emit_block(else_block);
        emit_stmt(else_stmt);
        LLVMBuildBr(builder, exit_block);
    }

    llvm_emit_block(exit_block);
}

void LLVM_Backend::emit_while(Ast_While *while_stmt) {
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

    LLVMBasicBlockRef head = LLVMCreateBasicBlockInContext(context, "loop_head");
    LLVMBasicBlockRef body = LLVMCreateBasicBlockInContext(context, "loop_body");
    LLVMBasicBlockRef tail = LLVMCreateBasicBlockInContext(context, "loop_tail");

    LLVMBuildBr(builder, head);

    // head
    llvm_emit_block(head);

    LLVMValueRef condition = emit_condition(while_stmt->cond);
    LLVMBuildCondBr(builder, condition, body, tail);

    // body
    llvm_emit_block(body);
    emit_block(while_stmt->block);
    LLVMBuildBr(builder, head);

    // tail
    llvm_emit_block(tail);
}

void LLVM_Backend::emit_for(Ast_For *for_stmt) {
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

    if (for_stmt->init) emit_stmt(for_stmt->init);

    LLVMBasicBlockRef head = llvm_block_new("loop_head");
    LLVMBasicBlockRef body = llvm_block_new("loop_body");
    LLVMBasicBlockRef tail = llvm_block_new("loop_tail");

    LLVMBuildBr(builder, head);

    // head
    llvm_emit_block(head);

    if (for_stmt->cond) {
        LLVMValueRef condition = emit_condition(for_stmt->cond);
        LLVMBuildCondBr(builder, condition, body, tail);
    } else {
        LLVMBuildBr(builder, body);
    }

    // body
    llvm_emit_block(body);
    emit_block(for_stmt->block);

    if (for_stmt->iterator) emit_expr(for_stmt->iterator);

    LLVMBuildBr(builder, head);

    // tail
    llvm_emit_block(tail);
}

void LLVM_Backend::emit_stmt(Ast_Stmt *stmt) {
    switch (stmt->kind) {
    case AST_EXPR_STMT:
    {
        Ast_Expr_Stmt *expr_stmt = static_cast<Ast_Expr_Stmt*>(stmt);
        Ast_Expr *expr = expr_stmt->expr;
        LLVM_Value value = emit_expr(expr);
        break;
    }

    case AST_DECL_STMT:
    {
        Ast_Decl_Stmt *decl_stmt = static_cast<Ast_Decl_Stmt*>(stmt);
        Ast_Decl *decl = decl_stmt->decl;
        if (decl->kind == AST_VAR) {
            Ast_Var *var_node = static_cast<Ast_Var*>(decl);
            LLVM_Var *var = llvm_alloc(LLVM_Var);
            var->name = var_node->name;
            var->decl = decl;
            var->type = emit_type(var_node->type_info);
            var->alloca = LLVMBuildAlloca(builder, var->type, (char *)var->name->data);
            current_proc->named_values.push(var);

            // printf("%s %d\n", var->name->data, var_node->type_info->builtin_kind);

            if (!var_node->init) {
                LLVMValueRef null_value = LLVMConstNull(var->type);
                LLVMBuildStore(builder, null_value, var->alloca);
            }

            if (var_node->init) {
                Ast_Expr *init = var_node->init;
                if (init->kind == AST_COMPOUND_LITERAL) {
                    Ast_Compound_Literal *literal = static_cast<Ast_Compound_Literal*>(init);
                    if (var_node->type_info->is_struct_type()) {
                        LLVM_Struct *llvm_struct = lookup_struct(var_node->type_info->decl->name);
                        for (int i = 0; i < literal->elements.count; i++) {
                            Ast_Expr *elem = literal->elements[i];
                            LLVM_Value value = emit_expr(elem);
                            LLVMValueRef field_addr = LLVMBuildStructGEP2(builder, llvm_struct->type, var->alloca, (unsigned)i, "fieldaddr_tmp");
                            LLVMBuildStore(builder, value.value, field_addr);
                        }
                    } else if (var_node->type_info->is_array_type()) {
                        LLVMTypeRef array_type = emit_type(var_node->type_info);

                        Auto_Array<LLVMValueRef> values;
                        for (int i = 0; i < literal->elements.count; i++) {
                            Ast_Expr *elem = literal->elements[i];
                            LLVM_Value value = emit_expr(elem);
                            values.push(value.value);
                        }

                        LLVMTypeRef element_type = LLVMGetElementType(array_type);
                        LLVMValueRef const_array = LLVMConstArray2(element_type, values.data, (u64)values.count);

                        // LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0)};
                        // LLVMValueRef ptr = LLVMBuildGEP2(builder, array_type, const_array, indices, 1, "arrayinitptr");

                        LLVMBuildStore(builder, const_array, var->alloca);
                    }
                } else {
                    LLVM_Value init_value = emit_expr(init);
                    LLVMBuildStore(builder, init_value.value, var->alloca);
                }
            }
        }
        break;
    }

    case AST_IF:
    {
        Ast_If *if_stmt = static_cast<Ast_If*>(stmt);
        emit_if(if_stmt);
        break;
    }

    case AST_SWITCH:
    {
        break;
    }

    case AST_WHILE:
    {
        Ast_While *while_stmt = static_cast<Ast_While*>(stmt);
        emit_while(while_stmt);
        break;
    }

    case AST_FOR:
    {
        Ast_For *for_stmt = static_cast<Ast_For*>(stmt);
        emit_for(for_stmt);
        break;
    }

    case AST_BLOCK:
    {
        Ast_Block *block = static_cast<Ast_Block*>(stmt);
        emit_block(block);
        break;
    }

    case AST_RETURN:
    {
        Ast_Return *return_stmt = static_cast<Ast_Return*>(stmt);
        if (return_stmt->expr) {
            LLVM_Value ret = emit_expr(return_stmt->expr);
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

LLVM_Procedure *LLVM_Backend::emit_procedure(Ast_Proc *proc) {
    Ast_Proc_Type_Info *proc_type = static_cast<Ast_Proc_Type_Info*>(proc->type_info);

    LLVM_Procedure *llvm_proc = llvm_alloc(LLVM_Procedure);
    llvm_proc->name = proc->name;
    llvm_proc->proc = proc;

    if (proc->parameters.count) llvm_proc->parameter_types.reserve(proc_type->parameters.count);

    for (int i = 0; i < proc_type->parameters.count; i++) {
        Ast_Type_Info *type_info = proc_type->parameters[i];
        LLVMTypeRef type = emit_type(type_info);
        llvm_proc->parameter_types.push(type);
    }
    llvm_proc->return_type = emit_type(proc_type->return_type);

    llvm_proc->type = LLVMFunctionType(llvm_proc->return_type, llvm_proc->parameter_types.data, (int)llvm_proc->parameter_types.count, false);

    llvm_proc->value = LLVMAddFunction(module, (char *)llvm_proc->name->data, llvm_proc->type);

    llvm_proc->builder = LLVMCreateBuilder();

    return llvm_proc;
}

void LLVM_Backend::emit_procedure_body(LLVM_Procedure *procedure) {
    Ast_Proc *proc = procedure->proc;
    current_proc = procedure;

    builder = procedure->builder;

    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context, procedure->value, "entry");
    procedure->entry = entry;
    LLVMPositionBuilderAtEnd(procedure->builder, entry);

    for (int i = 0; i < proc->parameters.count; i++) {
        Ast_Param *param = proc->parameters[i];
        LLVMValueRef param_value = LLVMGetParam(procedure->value, i);
        LLVMTypeRef param_type = procedure->parameter_types[i];

        LLVM_Var *var = llvm_alloc(LLVM_Var);
        var->name = param->name;
        var->decl = param;
        var->type = emit_type(param->type_info);
        var->alloca = LLVMBuildAlloca(procedure->builder, param_type, (char *)var->name->data);
        procedure->named_values.push(var);

        LLVMBuildStore(procedure->builder, param_value, var->alloca);
    }

    Ast_Proc_Type_Info *proc_type = static_cast<Ast_Proc_Type_Info*>(procedure->proc->type_info);

    emit_block(proc->block);

    if (proc_type->return_type == type_void) {
        LLVMBasicBlockRef last_block = LLVMGetLastBasicBlock(procedure->value);
        LLVMValueRef last_instr = LLVMGetLastInstruction(last_block);

        bool need_return = false;
        if (!last_instr) {
            need_return = true;
        } else {
            LLVMOpcode opcode = LLVMGetInstructionOpcode(last_instr);
            if (opcode == LLVMRet) need_return = false;
        }

        if (need_return) {
            LLVMBuildRetVoid(builder); 
        }
    }
}

LLVM_Struct *LLVM_Backend::emit_struct(Ast_Struct *struct_decl) {
    LLVM_Struct *llvm_struct = llvm_alloc(LLVM_Struct);
    llvm_struct->name = struct_decl->name;

    llvm_struct->type = LLVMStructCreateNamed(LLVMGetGlobalContext(), (char *)llvm_struct->name->data);

    llvm_struct->element_types.reserve(struct_decl->fields.count);

    for (int i = 0; i < struct_decl->fields.count; i++) {
        Ast_Struct_Field *field = struct_decl->fields[i];
        LLVMTypeRef field_type = emit_type(field->type_info);
        llvm_struct->element_types.push(field_type);
    }

    LLVMStructSetBody(llvm_struct->type, llvm_struct->element_types.data, (unsigned)llvm_struct->element_types.count, false);

    return llvm_struct;
}

void LLVM_Backend::emit_decl(Ast_Decl *decl) {
    switch (decl->kind) {
    case AST_PROC:
    {
        Ast_Proc *proc = static_cast<Ast_Proc*>(decl);
        LLVM_Procedure *llvm_proc = emit_procedure(proc);
        global_procedures.push(llvm_proc);
        break;
    }
    case AST_STRUCT:
    {
        Ast_Struct *struct_decl = static_cast<Ast_Struct*>(decl);
        LLVM_Struct *llvm_struct = emit_struct(struct_decl);
        global_structs.push(llvm_struct);
        break;
    }
    }
}
