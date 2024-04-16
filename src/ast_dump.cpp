#include "ast_dump.h"

void Ast_Dump::dump(Ast *node) {
    if (!node) return;
    switch (node->type) {
    case AST_ROOT:
    {
        Ast_Root *root = static_cast<Ast_Root*>(node);
        for (int i = 0; i < root->declarations.count; i++) {
            dump(root->declarations[i]);
        }
        break;
    }
    case AST_BLOCK:
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
    case AST_STRUCT:
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
    case AST_STRUCT_FIELD:
    {
        Ast_Struct_Field *field = static_cast<Ast_Struct_Field *>(node);
        print("STRUCT FIELD %s\n", field->ident->name->name);
        indent();
        print("'%s'\n", type_definition_to_string(field->type_definition));
        outdent();
        break;
    }
    case AST_PROCEDURE:
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
    case AST_VARIABLE:
    {
        Ast_Variable *variable = static_cast<Ast_Variable *>(node);
        print("VARIABLE %s: '%s'\n", variable->ident->name->name, type_definition_to_string(variable->type_definition));
        break;
    }
    case AST_DECLARATION_STATEMENT:
    {
        Ast_Declaration_Statement *declaration_statement = static_cast<Ast_Declaration_Statement *>(node);
        print("DECLARATION STATEMENT\n");
        indent();
        dump(declaration_statement->declaration);
        outdent();
        break;
    }
    case AST_EXPRESSION_STATEMENT:
    {
        Ast_Expression_Statement *expression_statement = static_cast<Ast_Expression_Statement *>(node);
        print("EXPRESSION STATEMENT\n");
        indent();
        dump(expression_statement->expression);
        outdent();
        break;
    }
    case AST_WHILE:
    {
        Ast_While *while_statement = static_cast<Ast_While *>(node);
        print("WHILE STATEMENT\n");
        indent();
        dump(while_statement->condition);
        dump(while_statement->block);
        outdent();
        break;
    }
    case AST_IF:
    {
        Ast_If *if_statement = static_cast<Ast_If *>(node);
        print("IF STATEMENT\n");
        indent();
        dump(if_statement->condition);
        dump(if_statement->block);
        outdent();
        break;
    }
    case AST_RETURN:
    {
        Ast_Return *return_statement = static_cast<Ast_Return *>(node);
        print("RETURN STATEMENT\n");
        indent();
        dump(return_statement->expression);
        outdent();
        break;
    }
    case AST_BINARY_EXPRESSION:
    {
        Ast_Binary_Expression *binary_expression = static_cast<Ast_Binary_Expression *>(node);
        print("BINARY '%s'\n", token_type_to_string(binary_expression->op));
        indent();
        dump(binary_expression->lhs);
        dump(binary_expression->rhs);
        outdent();
        break;
    }
    case AST_UNARY_EXPRESSION:
    {
        Ast_Unary_Expression *unary_expression = static_cast<Ast_Unary_Expression *>(node);
        print("UNARY '%s'\n", token_type_to_string(unary_expression->op));
        indent();
        dump(unary_expression->expression);
        outdent();
        break;
    }
    case AST_LITERAL:
    {
        Ast_Literal *literal = static_cast<Ast_Literal *>(node);
        if (literal->literal_flags & LITERAL_FLOAT) print("FLOAT %f\n", literal->float_value);
        else if (literal->literal_flags & LITERAL_NUMBER) print("INTEGER %d\n", literal->int_value);
        else if (literal->literal_flags & LITERAL_STRING) print("STRING \"%s\"\n", literal->string_value);
        break;
    }
    case AST_IDENT:
    {
        Ast_Ident *ident = static_cast<Ast_Ident *>(node);
        print("IDENT %s\n", ident->name->name);
        break;
    }
    }
}
