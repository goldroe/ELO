#include "ast_dump.h"

void Ast_Dump::dump_ast(Ast *root) {
    dump(root);
    if (dump_file != stdout && dump_file != nullptr) {
        fclose(dump_file);
    }
}

void Ast_Dump::dump(Ast *node) {
    if (!node) return;
    switch (node->kind) {
    case AstKind_Root:
    {
        Ast_Root *root = static_cast<Ast_Root*>(node);
        for (int i = 0; i < root->declarations.count; i++) {
            dump(root->declarations[i]);
        }
        break;
    }
    case AstKind_Block:
    {
        Ast_Block *block = static_cast<Ast_Block *>(node);
        print("Block\n");
        indent();
        for (int i = 0; i < block->statements.count; i++) {
            dump(block->statements[i]);
        }
        outdent();
        break;
    }
    case AstKind_Struct:
    {
        Ast_Struct_Declaration *struct_declaration = static_cast<Ast_Struct_Declaration *>(node);
        print("StructDecl %s\n", struct_declaration->ident->name->name);

        indent();
        for (int i = 0; i <struct_declaration->fields.count; i++) {
            dump(struct_declaration->fields[i]);
        }
        outdent();
        break;
    }
    case AstKind_StructField:
    {
        Ast_Struct_Field *field = static_cast<Ast_Struct_Field *>(node);
        print("StructField %s\n", field->name->name);
        indent();
        print("'%s'\n", type_definition_to_string(field->type_definition));
        outdent();
        break;
    }
    case AstKind_Enum:
    {
        Ast_Enum_Declaration *enum_decl = static_cast<Ast_Enum_Declaration*>(node);
        print("EnumDecl %s\n", enum_decl->ident->name->name);
        indent();
        for (int i = 0; i < enum_decl->fields.count; i++) {
            Ast_Enum_Field *field = enum_decl->fields[i];
            dump(field);
        }
        outdent();
        break;
    }
    case AstKind_EnumField:
    {
        Ast_Enum_Field *field = static_cast<Ast_Enum_Field*>(node);
        print("'%s' %d\n", field->name->name, field->value);
        break;
    }
    case AstKind_Procedure:
    {
        Ast_Procedure_Declaration *procedure = static_cast<Ast_Procedure_Declaration *>(node);
        print("ProcedureDecl %s: '%s'\n", procedure->ident->name->name, type_definition_to_string(procedure->return_type));
        indent();
        for (int i = 0; i < procedure->parameters.count; i++) {
            Ast_Variable *variable = procedure->parameters[i];
            dump(variable);
        }
        dump(procedure->body);
        outdent();
        break;
    }
    case AstKind_Variable:
    {
        Ast_Variable *variable = static_cast<Ast_Variable *>(node);
        print("VariableDecl %s: '%s'\n", variable->ident->name->name, type_definition_to_string(variable->type_definition));
        indent();
        dump(variable->initializer);
        outdent();
        break;
    }
    case AstKind_BlockStatement:
    {
        Ast_Block_Statement *block_statement = static_cast<Ast_Block_Statement*>(node);
        print("BlockStmt\n");
        indent();
        dump(block_statement->block);
        outdent();
        break;
    }
    case AstKind_DeclarationStatement:
    {
        Ast_Declaration_Statement *declaration_statement = static_cast<Ast_Declaration_Statement *>(node);
        print("DeclStmt\n");
        indent();
        dump(declaration_statement->declaration);
        outdent();
        break;
    }
    case AstKind_ExpressionStatement:
    {
        Ast_Expression_Statement *expression_statement = static_cast<Ast_Expression_Statement *>(node);
        print("ExprStmt\n");
        indent();
        dump(expression_statement->expression);
        outdent();
        break;
    }
    case AstKind_While:
    {
        Ast_While *while_statement = static_cast<Ast_While *>(node);
        print("WhileStmt\n");
        indent();
        dump(while_statement->condition);
        dump(while_statement->block);
        outdent();
        break;
    }
    case AstKind_For:
    {
        Ast_For *for_statement = static_cast<Ast_For*>(node);
        print("ForStmt\n");
        indent();
        dump(for_statement->iterator);
        dump(for_statement->range);
        dump(for_statement->block);
        outdent();
        break;
    }
    case AstKind_If:
    {
        Ast_If *if_statement = static_cast<Ast_If *>(node);
        print("IfStmt\n");
        indent();
        dump(if_statement->condition);
        dump(if_statement->block);
        dump(if_statement->next);
        outdent();
        break;
    }
    case AstKind_Return:
    {
        Ast_Return *return_statement = static_cast<Ast_Return *>(node);
        print("ReturnStmt\n");
        indent();
        dump(return_statement->expression);
        outdent();
        break;
    }
    case AstKind_IndexExpression:
    {
        Ast_Index_Expression *index_expression = static_cast<Ast_Index_Expression*>(node);
        print("IndexExpr\n");
        indent();
        dump(index_expression->array);
        dump(index_expression->index);
        outdent();
        break;
    }
    case AstKind_FieldExpression:
    {
        Ast_Field_Expression *field_expression = static_cast<Ast_Field_Expression*>(node);
        print("FieldExpr\n");
        indent();
        dump(field_expression->operand);
        dump(field_expression->field);
        outdent();
        break;
    }
    case AstKind_CallExpression:
    {
        Ast_Call_Expression *call_expression = static_cast<Ast_Call_Expression*>(node);
        print("CallExpr\n");
        indent();
        dump(call_expression->operand);
        for (int i = 0; i < call_expression->arguments.count; i++) {
            Ast_Expression *argument = call_expression->arguments[i];
            dump(argument);
        }
        outdent();
        break;
    }
    case AstKind_BinaryExpression:
    {
        Ast_Binary_Expression *binary_expression = static_cast<Ast_Binary_Expression *>(node);
        print("BinaryExpr '%s'\n", token_type_to_string(binary_expression->op));
        indent();
        dump(binary_expression->lhs);
        dump(binary_expression->rhs);
        outdent();
        break;
    }
    case AstKind_UnaryExpression:
    {
        Ast_Unary_Expression *unary_expression = static_cast<Ast_Unary_Expression *>(node);
        print("UnaryExpr '%s'\n", token_type_to_string(unary_expression->op));
        indent();
        dump(unary_expression->expression);
        outdent();
        break;
    }
    case AstKind_RangeExpression:
    {
        Ast_Range_Expression *range_expression = static_cast<Ast_Range_Expression*>(node);
        print("RangeExpr\n");
        indent();
        dump(range_expression->first);
        dump(range_expression->last);
        outdent();
        break;
    }
    case AstKind_Literal:
    {
        Ast_Literal *literal = static_cast<Ast_Literal *>(node);
        if (literal->literal_flags & LiteralFlag_Float) print("FloatLit %f\n", literal->float_value);
        else if (literal->literal_flags & LiteralFlag_Number) print("IntLit %llu\n", literal->int_value);
        else if (literal->literal_flags & LiteralFlag_String) print("StringLit \"%s\"\n", literal->string_value);
        break;
    }
    case AstKind_Ident:
    {
        Ast_Ident *ident = static_cast<Ast_Ident *>(node);
        print("Ident %s\n", ident->name->name);
        break;
    }
    }
}
