#include "ast_dump.h"

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
        print("BLOCK\n");
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
        print("STRUCT %s\n", struct_declaration->ident->name->name);

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
        print("STRUCT FIELD %s\n", field->name->name);
        indent();
        print("'%s'\n", type_definition_to_string(field->type_definition));
        outdent();
        break;
    }
    case AstKind_Procedure:
    {
        Ast_Procedure_Declaration *procedure = static_cast<Ast_Procedure_Declaration *>(node);
        print("PROCEDURE %s: '%s'\n", procedure->ident->name->name, type_definition_to_string(procedure->return_type));
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
        print("VARIABLE %s: '%s'\n", variable->ident->name->name, type_definition_to_string(variable->type_definition));
        indent();
        dump(variable->initializer);
        outdent();
        break;
    }
    case AstKind_DeclarationStatement:
    {
        Ast_Declaration_Statement *declaration_statement = static_cast<Ast_Declaration_Statement *>(node);
        print("DECLARATION STATEMENT\n");
        indent();
        dump(declaration_statement->declaration);
        outdent();
        break;
    }
    case AstKind_ExpressionStatement:
    {
        Ast_Expression_Statement *expression_statement = static_cast<Ast_Expression_Statement *>(node);
        print("EXPRESSION STATEMENT\n");
        indent();
        dump(expression_statement->expression);
        outdent();
        break;
    }
    case AstKind_While:
    {
        Ast_While *while_statement = static_cast<Ast_While *>(node);
        print("WHILE STATEMENT\n");
        indent();
        dump(while_statement->condition);
        dump(while_statement->block);
        outdent();
        break;
    }
    case AstKind_If:
    {
        Ast_If *if_statement = static_cast<Ast_If *>(node);
        print("IF STATEMENT\n");
        indent();
        dump(if_statement->condition);
        dump(if_statement->block);
        outdent();
        break;
    }
    case AstKind_Return:
    {
        Ast_Return *return_statement = static_cast<Ast_Return *>(node);
        print("RETURN STATEMENT\n");
        indent();
        dump(return_statement->expression);
        outdent();
        break;
    }
    case AstKind_BinaryExpression:
    {
        Ast_Binary_Expression *binary_expression = static_cast<Ast_Binary_Expression *>(node);
        print("BINARY '%s'\n", token_type_to_string(binary_expression->op));
        indent();
        dump(binary_expression->lhs);
        dump(binary_expression->rhs);
        outdent();
        break;
    }
    case AstKind_UnaryExpression:
    {
        Ast_Unary_Expression *unary_expression = static_cast<Ast_Unary_Expression *>(node);
        print("UNARY '%s'\n", token_type_to_string(unary_expression->op));
        indent();
        dump(unary_expression->expression);
        outdent();
        break;
    }
    case AstKind_Literal:
    {
        Ast_Literal *literal = static_cast<Ast_Literal *>(node);
        if (literal->literal_flags & LiteralFlag_Float) print("FLOAT %f\n", literal->float_value);
        else if (literal->literal_flags & LiteralFlag_Number) print("INTEGER %llu\n", literal->int_value);
        else if (literal->literal_flags & LiteralFlag_String) print("STRING \"%s\"\n", literal->string_value);
        break;
    }
    case AstKind_Ident:
    {
        Ast_Ident *ident = static_cast<Ast_Ident *>(node);
        print("IDENT %s\n", ident->name->name);
        break;
    }
    }
}
