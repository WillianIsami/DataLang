/*
 * DataLang - Parser Main
 * Funções de liberação de memória, visualização da AST e função main
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

// Declarações externas (do lexer.c)
extern AFD* create_datalang_afd_from_afn();
extern TokenStream* tokenize(const char* input, AFD* afd);
extern void free_afd(AFD* afd);
extern void free_token_stream(TokenStream* stream);

// ==================== LIBERAÇÃO DA AST ====================

void free_ast(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.decl_count; i++) {
                free_ast(node->program.declarations[i]);
            }
            free(node->program.declarations);
            break;
            
        case AST_LET_DECL:
            free(node->let_decl.name);
            free_ast(node->let_decl.type_annotation);
            free_ast(node->let_decl.initializer);
            break;
            
        case AST_FN_DECL:
            free(node->fn_decl.name);
            for (int i = 0; i < node->fn_decl.param_count; i++) {
                free_ast(node->fn_decl.params[i]);
            }
            free(node->fn_decl.params);
            free_ast(node->fn_decl.return_type);
            free_ast(node->fn_decl.body);
            break;
            
        case AST_DATA_DECL:
            free(node->data_decl.name);
            for (int i = 0; i < node->data_decl.field_count; i++) {
                free_ast(node->data_decl.fields[i]);
            }
            free(node->data_decl.fields);
            break;
            
        case AST_FIELD_DECL:
            free(node->field_decl.field_name);
            free_ast(node->field_decl.field_type);
            break;
            
        case AST_PARAM:
            free(node->param.param_name);
            free_ast(node->param.param_type);
            break;
            
        case AST_IMPORT_DECL:
            free(node->import_decl.module_path);
            if (node->import_decl.alias) free(node->import_decl.alias);
            break;
            
        case AST_EXPORT_DECL:
            free(node->export_decl.export_name);
            break;
            
        case AST_IF_STMT:
            free_ast(node->if_stmt.condition);
            free_ast(node->if_stmt.then_block);
            free_ast(node->if_stmt.else_block);
            break;
            
        case AST_FOR_STMT:
            free(node->for_stmt.iterator);
            free_ast(node->for_stmt.iterable);
            free_ast(node->for_stmt.body);
            break;
            
        case AST_RETURN_STMT:
            free_ast(node->return_stmt.value);
            break;
            
        case AST_EXPR_STMT:
            free_ast(node->expr_stmt.expression);
            break;
            
        case AST_BLOCK:
            for (int i = 0; i < node->block.stmt_count; i++) {
                free_ast(node->block.statements[i]);
            }
            free(node->block.statements);
            break;
            
        case AST_BINARY_EXPR:
            free_ast(node->binary_expr.left);
            free_ast(node->binary_expr.right);
            break;
            
        case AST_UNARY_EXPR:
            free_ast(node->unary_expr.operand);
            break;
            
        case AST_CALL_EXPR:
            free_ast(node->call_expr.callee);
            for (int i = 0; i < node->call_expr.arg_count; i++) {
                free_ast(node->call_expr.arguments[i]);
            }
            free(node->call_expr.arguments);
            break;
            
        case AST_INDEX_EXPR:
            free_ast(node->index_expr.object);
            free_ast(node->index_expr.index);
            break;
            
        case AST_MEMBER_EXPR:
            free_ast(node->member_expr.object);
            free(node->member_expr.member);
            break;
            
        case AST_ASSIGN_EXPR:
            free_ast(node->assign_expr.target);
            free_ast(node->assign_expr.value);
            break;
            
        case AST_LAMBDA_EXPR:
            for (int i = 0; i < node->lambda_expr.lambda_param_count; i++) {
                free_ast(node->lambda_expr.lambda_params[i]);
            }
            free(node->lambda_expr.lambda_params);
            free_ast(node->lambda_expr.lambda_body);
            break;
            
        case AST_PIPELINE_EXPR:
            for (int i = 0; i < node->pipeline_expr.stage_count; i++) {
                free_ast(node->pipeline_expr.stages[i]);
            }
            free(node->pipeline_expr.stages);
            break;
            
        case AST_FILTER_TRANSFORM:
            free_ast(node->filter_transform.filter_predicate);
            break;
            
        case AST_MAP_TRANSFORM:
            free_ast(node->map_transform.map_function);
            break;
            
        case AST_REDUCE_TRANSFORM:
            free_ast(node->reduce_transform.initial_value);
            free_ast(node->reduce_transform.reducer);
            break;
            
        case AST_SELECT_TRANSFORM:
            for (int i = 0; i < node->select_transform.column_count; i++) {
                free(node->select_transform.columns[i]);
            }
            free(node->select_transform.columns);
            break;
            
        case AST_GROUPBY_TRANSFORM:
            for (int i = 0; i < node->groupby_transform.group_column_count; i++) {
                free(node->groupby_transform.group_columns[i]);
            }
            free(node->groupby_transform.group_columns);
            break;
            
        case AST_AGGREGATE_TRANSFORM:
            for (int i = 0; i < node->aggregate_transform.agg_arg_count; i++) {
                free_ast(node->aggregate_transform.agg_args[i]);
            }
            free(node->aggregate_transform.agg_args);
            break;
            
        case AST_LITERAL:
            if (node->literal.literal_type == TOKEN_STRING && node->literal.string_value) {
                free(node->literal.string_value);
            }
            break;
            
        case AST_IDENTIFIER:
            free(node->identifier.id_name);
            break;
            
        case AST_ARRAY_LITERAL:
            for (int i = 0; i < node->array_literal.element_count; i++) {
                free_ast(node->array_literal.elements[i]);
            }
            free(node->array_literal.elements);
            break;
            
        case AST_LOAD_EXPR:
            free(node->load_expr.file_path);
            break;
            
        case AST_SAVE_EXPR:
            free_ast(node->save_expr.data);
            free(node->save_expr.save_path);
            break;
            
        case AST_RANGE_EXPR:
            free_ast(node->range_expr.range_start);
            free_ast(node->range_expr.range_end);
            break;
            
        case AST_TYPE:
            if (node->type_node.type_name) free(node->type_node.type_name);
            free_ast(node->type_node.inner_type);
            if (node->type_node.tuple_types) {
                for (int i = 0; i < node->type_node.tuple_type_count; i++) {
                    free_ast(node->type_node.tuple_types[i]);
                }
                free(node->type_node.tuple_types);
            }
            break;
            
        default:
            break;
    }
    
    free(node);
}

// ==================== VISUALIZAÇÃO DA AST ====================

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

static const char* ast_node_type_name(ASTNodeType type) {
    switch (type) {
        case AST_PROGRAM: return "Program";
        case AST_LET_DECL: return "LetDecl";
        case AST_FN_DECL: return "FnDecl";
        case AST_DATA_DECL: return "DataDecl";
        case AST_IMPORT_DECL: return "ImportDecl";
        case AST_EXPORT_DECL: return "ExportDecl";
        case AST_IF_STMT: return "IfStmt";
        case AST_FOR_STMT: return "ForStmt";
        case AST_RETURN_STMT: return "ReturnStmt";
        case AST_EXPR_STMT: return "ExprStmt";
        case AST_BLOCK: return "Block";
        case AST_BINARY_EXPR: return "BinaryExpr";
        case AST_UNARY_EXPR: return "UnaryExpr";
        case AST_CALL_EXPR: return "CallExpr";
        case AST_INDEX_EXPR: return "IndexExpr";
        case AST_MEMBER_EXPR: return "MemberExpr";
        case AST_ASSIGN_EXPR: return "AssignExpr";
        case AST_LAMBDA_EXPR: return "LambdaExpr";
        case AST_PIPELINE_EXPR: return "PipelineExpr";
        case AST_FILTER_TRANSFORM: return "FilterTransform";
        case AST_MAP_TRANSFORM: return "MapTransform";
        case AST_REDUCE_TRANSFORM: return "ReduceTransform";
        case AST_SELECT_TRANSFORM: return "SelectTransform";
        case AST_GROUPBY_TRANSFORM: return "GroupByTransform";
        case AST_AGGREGATE_TRANSFORM: return "AggregateTransform";
        case AST_LITERAL: return "Literal";
        case AST_IDENTIFIER: return "Identifier";
        case AST_ARRAY_LITERAL: return "ArrayLiteral";
        case AST_LOAD_EXPR: return "LoadExpr";
        case AST_SAVE_EXPR: return "SaveExpr";
        case AST_RANGE_EXPR: return "RangeExpr";
        case AST_TYPE: return "Type";
        case AST_PARAM: return "Param";
        case AST_FIELD_DECL: return "FieldDecl";
        default: return "Unknown";
    }
}

void print_ast(ASTNode* node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("├─ NULL\n");
        return;
    }
    
    print_indent(indent);
    printf("├─ %s", ast_node_type_name(node->type));
    
    switch (node->type) {
        case AST_PROGRAM:
            printf(" (%d declarações)\n", node->program.decl_count);
            for (int i = 0; i < node->program.decl_count; i++) {
                if (node->program.declarations[i]) {
                    print_ast(node->program.declarations[i], indent + 1);
                } else {
                    print_indent(indent + 1);
                    printf("├─ NULL\n");
                }
            }
            break;
            
        case AST_LET_DECL:
            if (node->let_decl.name) {
                printf(" '%s'\n", node->let_decl.name);
            } else {
                printf(" NULL\n");
            }
            
            if (node->let_decl.type_annotation) {
                print_indent(indent + 1);
                printf("├─ Tipo:\n");
                print_ast(node->let_decl.type_annotation, indent + 2);
            }
            
            print_indent(indent + 1);
            printf("├─ Valor:\n");
            if (node->let_decl.initializer) {
                print_ast(node->let_decl.initializer, indent + 2);
            } else {
                print_indent(indent + 2);
                printf("├─ NULL\n");
            }
            break;
            
        case AST_FN_DECL:
            printf(" '%s' (%d parâmetros)\n", node->fn_decl.name, node->fn_decl.param_count);
            if (node->fn_decl.param_count > 0) {
                print_indent(indent + 1);
                printf("├─ Parâmetros:\n");
                for (int i = 0; i < node->fn_decl.param_count; i++) {
                    print_ast(node->fn_decl.params[i], indent + 2);
                }
            }
            if (node->fn_decl.return_type) {
                print_indent(indent + 1);
                printf("├─ Tipo de retorno:\n");
                print_ast(node->fn_decl.return_type, indent + 2);
            }
            print_indent(indent + 1);
            printf("├─ Corpo:\n");
            print_ast(node->fn_decl.body, indent + 2);
            break;
            
        case AST_BINARY_EXPR: {
            const char* op_str = "?";
            switch (node->binary_expr.op) {
                case BINOP_ADD: op_str = "+"; break;
                case BINOP_SUB: op_str = "-"; break;
                case BINOP_MUL: op_str = "*"; break;
                case BINOP_DIV: op_str = "/"; break;
                case BINOP_MOD: op_str = "%"; break;
                case BINOP_EQ: op_str = "=="; break;
                case BINOP_NEQ: op_str = "!="; break;
                case BINOP_LT: op_str = "<"; break;
                case BINOP_LTE: op_str = "<="; break;
                case BINOP_GT: op_str = ">"; break;
                case BINOP_GTE: op_str = ">="; break;
                case BINOP_AND: op_str = "&&"; break;
                case BINOP_OR: op_str = "||"; break;
                case BINOP_RANGE: op_str = ".."; break;
            }
            printf(" '%s'\n", op_str);
            print_indent(indent + 1);
            printf("├─ Esquerda:\n");
            print_ast(node->binary_expr.left, indent + 2);
            print_indent(indent + 1);
            printf("├─ Direita:\n");
            print_ast(node->binary_expr.right, indent + 2);
            break;
        }
            
        case AST_UNARY_EXPR: {
            const char* op_str = (node->unary_expr.op == UNOP_NEG) ? "-" : "!";
            printf(" '%s'\n", op_str);
            print_ast(node->unary_expr.operand, indent + 1);
            break;
        }
            
        case AST_CALL_EXPR:
            printf(" (%d argumentos)\n", node->call_expr.arg_count);
            print_indent(indent + 1);
            printf("├─ Função:\n");
            print_ast(node->call_expr.callee, indent + 2);
            if (node->call_expr.arg_count > 0) {
                print_indent(indent + 1);
                printf("├─ Argumentos:\n");
                for (int i = 0; i < node->call_expr.arg_count; i++) {
                    print_ast(node->call_expr.arguments[i], indent + 2);
                }
            }
            break;

        case AST_MEMBER_EXPR:
            if (node->member_expr.member) {
                printf(" '%s'\n", node->member_expr.member);
            } else {
                printf(" (null)\n");
            }
            print_indent(indent + 1);
            printf("├─ Objeto:\n");
            print_ast(node->member_expr.object, indent + 2);
            break;
            
        case AST_PIPELINE_EXPR:
            printf(" (%d estágios)\n", node->pipeline_expr.stage_count);
            for (int i = 0; i < node->pipeline_expr.stage_count; i++) {
                print_indent(indent + 1);
                printf("├─ Estágio %d:\n", i + 1);
                print_ast(node->pipeline_expr.stages[i], indent + 2);
            }
            break;
            
        case AST_LITERAL:
            printf(" [");
            // Verificar o tipo antes de acessar a union
            switch (node->literal.literal_type) {
                case TOKEN_INTEGER:
                    printf("INT: %lld", node->literal.int_value);
                    break;
                case TOKEN_FLOAT:
                    printf("FLOAT: %f", node->literal.float_value);
                    break;
                case TOKEN_STRING:
                    // O parser já removeu as aspas, então node->literal.string_value
                    // contém o valor bruto (ex: "DataLang"). Apenas imprimimos.
                    if (node->literal.string_value) {
                        printf("STRING: \"%s\"", node->literal.string_value);
                    } else {
                        printf("STRING: \"\"");
                    }
                    break;
                case TOKEN_BOOL_TYPE:
                    printf("BOOL: %s", node->literal.bool_value ? "true" : "false");
                    break;
                default:
                    printf("UNKNOWN TYPE: %d", node->literal.literal_type);
            }
            printf("]\n");
            break;
            
        case AST_IDENTIFIER:
            // O `node->identifier.id_name` NUNCA deve ser NULL se o parser
            // funcionou, mas se for um ponteiro lixo (corrupção de heap),
            // esta verificação simples pode não ser suficiente.
            if (node->identifier.id_name != NULL) {
                // Tenta imprimir de forma segura
                printf(" '%s'\n", node->identifier.id_name);
            } else {
                printf(" 'NULL_ID'\n");
            }
            break;
        
        // case AST_IDENTIFIER:
        //     if (node->identifier.id_name) {
        //         printf(" '%s'\n", node->identifier.id_name);
        //     } else {
        //         printf(" 'NULL_ID'\n");
        //     }
        //     break;
            
        case AST_BLOCK:
            printf(" (%d statements)\n", node->block.stmt_count);
            for (int i = 0; i < node->block.stmt_count; i++) {
                print_ast(node->block.statements[i], indent + 1);
            }
            break;
            
        case AST_IF_STMT:
            printf("\n");
            print_indent(indent + 1);
            printf("├─ Condição:\n");
            print_ast(node->if_stmt.condition, indent + 2);
            print_indent(indent + 1);
            printf("├─ Então:\n");
            print_ast(node->if_stmt.then_block, indent + 2);
            if (node->if_stmt.else_block) {
                print_indent(indent + 1);
                printf("├─ Senão:\n");
                print_ast(node->if_stmt.else_block, indent + 2);
            }
            break;
            
        case AST_TYPE:
            if (node->type_node.type_name) {
                printf(" '%s'\n", node->type_node.type_name);
            } else {
                printf("\n");
            }
            if (node->type_node.inner_type) {
                print_ast(node->type_node.inner_type, indent + 1);
            }
            break;
            
        case AST_PARAM:
            printf(" '%s'\n", node->param.param_name);
            if (node->param.param_type) {
                print_indent(indent + 1);
                printf("├─ Tipo:\n");
                print_ast(node->param.param_type, indent + 2);
            }
            break;
            
        case AST_DATA_DECL:
            printf(" '%s' (%d campos)\n", node->data_decl.name, node->data_decl.field_count);
            if (node->data_decl.field_count > 0) {
                print_indent(indent + 1);
                printf("├─ Campos:\n");
                for (int i = 0; i < node->data_decl.field_count; i++) {
                    print_ast(node->data_decl.fields[i], indent + 2);
                }
            }
            break;
            
        case AST_FIELD_DECL:
            printf(" '%s'\n", node->field_decl.field_name);
            if (node->field_decl.field_type) {
                print_indent(indent + 1);
                printf("├─ Tipo:\n");
                print_ast(node->field_decl.field_type, indent + 2);
            }
            break;
            
        case AST_ARRAY_LITERAL:
            printf(" [%d elementos]\n", node->array_literal.element_count);
            for (int i = 0; i < node->array_literal.element_count; i++) {
                print_ast(node->array_literal.elements[i], indent + 1);
            }
            break;
            
        case AST_RANGE_EXPR:
            printf("\n");
            print_indent(indent + 1);
            printf("├─ Início:\n");
            print_ast(node->range_expr.range_start, indent + 2);
            print_indent(indent + 1);
            printf("├─ Fim:\n");
            print_ast(node->range_expr.range_end, indent + 2);
            break;
            
        case AST_FOR_STMT:
            printf(" (iterador: '%s')\n", node->for_stmt.iterator);
            print_indent(indent + 1);
            printf("├─ Iterável:\n");
            print_ast(node->for_stmt.iterable, indent + 2);
            print_indent(indent + 1);
            printf("├─ Corpo:\n");
            print_ast(node->for_stmt.body, indent + 2);
            break;
            
        case AST_RETURN_STMT:
            printf("\n");
            if (node->return_stmt.value) {
                print_indent(indent + 1);
                printf("├─ Valor:\n");
                print_ast(node->return_stmt.value, indent + 2);
            }
            break;
            
        case AST_EXPR_STMT:
            printf("\n");
            print_ast(node->expr_stmt.expression, indent + 1);
            break;
            
        default:
            printf(" (não implementado)\n");
            break;
    }
}

static void json_indent(FILE* f, int indent) {
    for (int i = 0; i < indent; i++) fprintf(f, "  ");
}

void ast_to_json(ASTNode* node, FILE* out, int indent) {
    if (!node) {
        fprintf(out, "null");
        return;
    }

    json_indent(out, indent);
    fprintf(out, "{\n");

    // Tipo do nó
    json_indent(out, indent + 1);
    fprintf(out, "\"type\": \"%s\"", ast_node_type_name(node->type));

    // Para facilitar leitura, todas as propriedades virão depois de vírgulas
    fprintf(out, ",\n");

    switch (node->type) {

        case AST_PROGRAM: {
            json_indent(out, indent + 1);
            fprintf(out, "\"declarations\": [\n");

            for (int i = 0; i < node->program.decl_count; i++) {
                ast_to_json(node->program.declarations[i], out, indent + 2);
                if (i < node->program.decl_count - 1) fprintf(out, ",");
                fprintf(out, "\n");
            }

            json_indent(out, indent + 1);
            fprintf(out, "]");
            break;
        }

        case AST_LET_DECL:
            json_indent(out, indent + 1);
            fprintf(out, "\"name\": \"%s\",\n", node->let_decl.name);

            json_indent(out, indent + 1);
            fprintf(out, "\"type_annotation\": ");
            ast_to_json(node->let_decl.type_annotation, out, indent + 2);
            fprintf(out, ",\n");

            json_indent(out, indent + 1);
            fprintf(out, "\"initializer\": ");
            ast_to_json(node->let_decl.initializer, out, indent + 2);
            break;

        case AST_LITERAL:
            json_indent(out, indent + 1);
            fprintf(out, "\"value\": ");

            switch (node->literal.literal_type) {
                case TOKEN_INTEGER:
                    fprintf(out, "%lld", node->literal.int_value);
                    break;
                case TOKEN_FLOAT:
                    fprintf(out, "%f", node->literal.float_value);
                    break;
                case TOKEN_STRING:
                    fprintf(out, "\"%s\"", node->literal.string_value ? node->literal.string_value : "");
                    break;
                case TOKEN_BOOL_TYPE:
                    fprintf(out, node->literal.bool_value ? "true" : "false");
                    break;
                default:
                    fprintf(out, "\"UNKNOWN\"");
            }
            break;

        case AST_IDENTIFIER:
            json_indent(out, indent + 1);
            fprintf(out, "\"name\": \"%s\"", node->identifier.id_name);
            break;

        case AST_BINARY_EXPR:
            json_indent(out, indent + 1);
            fprintf(out, "\"operator\": \"%d\",\n", node->binary_expr.op);

            json_indent(out, indent + 1);
            fprintf(out, "\"left\": ");
            ast_to_json(node->binary_expr.left, out, indent + 2);
            fprintf(out, ",\n");

            json_indent(out, indent + 1);
            fprintf(out, "\"right\": ");
            ast_to_json(node->binary_expr.right, out, indent + 2);
            break;

        case AST_FN_DECL:
            json_indent(out, indent + 1);
            fprintf(out, "\"name\": \"%s\",\n", node->fn_decl.name);

            json_indent(out, indent + 1);
            fprintf(out, "\"params\": [\n");
            for (int i = 0; i < node->fn_decl.param_count; i++) {
                ast_to_json(node->fn_decl.params[i], out, indent + 2);
                if (i < node->fn_decl.param_count - 1) fprintf(out, ",");
                fprintf(out, "\n");
            }
            json_indent(out, indent + 1);
            fprintf(out, "],\n");

            json_indent(out, indent + 1);
            fprintf(out, "\"return_type\": ");
            ast_to_json(node->fn_decl.return_type, out, indent + 2);
            fprintf(out, ",\n");

            json_indent(out, indent + 1);
            fprintf(out, "\"body\": ");
            ast_to_json(node->fn_decl.body, out, indent + 2);
            break;

        default:
            json_indent(out, indent + 1);
            fprintf(out, "\"UNIMPLEMENTED\": true");
            break;
    }

    fprintf(out, "\n");
    json_indent(out, indent);
    fprintf(out, "}");
}

void save_ast_json(ASTNode* ast, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("Erro ao salvar JSON da AST!\n");
        return;
    }
    ast_to_json(ast, f, 0);
    fclose(f);
    printf("✓ AST exportada para %s\n", filename);
}


// ==================== FUNÇÃO MAIN ====================

void test_parser_with_code(const char* code) {
    printf("\n════════════════════════════════════════════════════════════\n");
    printf("Código DataLang:\n");
    printf("════════════════════════════════════════════════════════════\n");
    printf("%s\n", code);
    printf("════════════════════════════════════════════════════════════\n");
    
    // Etapa 1: Análise Léxica
    AFD* afd = create_datalang_afd_from_afn();
    if (!afd) {
        printf("Erro ao criar AFD\n");
        return;
    }
    
    TokenStream* tokens = tokenize(code, afd);
    if (!tokens) {
        printf("Erro na tokenização\n");
        free_afd(afd);
        return;
    }
    
    // Etapa 2: Análise Sintática (Parsing)
    Parser* parser = create_parser(tokens);
    if (!parser) {
        printf("Erro ao criar parser\n");
        free_token_stream(tokens);
        free_afd(afd);
        return;
    }
    
    ASTNode* ast = parse(parser);
    
    // Visualiza a AST
    if (ast && !parser->had_error) {
        printf("\n╔════════════════════════════════════════════════════════════╗\n");
        printf("║                   ÁRVORE SINTÁTICA ABSTRATA                ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n\n");
        print_ast(ast, 0);
        save_ast_json(ast, "AST.json");
        printf("\n✓ AST construída com sucesso!\n");
    }
    
    // Limpeza
    if (ast) free_ast(ast);
    free_parser(parser);
    free_token_stream(tokens);
    free_afd(afd);
}

// int main(int argc, char** argv) {
//     printf("\n╔════════════════════════════════════════════════════════════╗\n");
//     printf("║           COMPILADOR DATALANG - PARSER LL(1)              ║\n");
//     printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
//     if (argc > 1) {
//         // Modo arquivo
//         FILE* file = fopen(argv[1], "r");
//         if (!file) {
//             fprintf(stderr, "Erro: Não foi possível abrir o arquivo '%s'\n", argv[1]);
//             return 1;
//         }
        
//         fseek(file, 0, SEEK_END);
//         long size = ftell(file);
//         fseek(file, 0, SEEK_SET);
        
//         char* code = malloc(size + 1);
//         fread(code, 1, size, file);
//         code[size] = '\0';
//         fclose(file);
        
//         test_parser_with_code(code);
//         free(code);
//     } else {
//         // Modo teste
//         const char* test_code = 
//             "let x = 4;\n"
//             "let name = \"DataLang\";\n"
//             "\n"
//             "fn soma(a: Int, b: Int) -> Int {\n"
//             "    return a + b;\n"
//             "}\n"
//             "\n"
//             "let resultado = soma(5, 3);\n";
        
//         test_parser_with_code(test_code);
        
//         printf("\n\nPara testar com um arquivo:\n");
//         printf("  %s <arquivo.datalang>\n\n", argv[0]);
//     }
    
//     return 0;
// }