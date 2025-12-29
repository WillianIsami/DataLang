/*
 * DataLang - Parser Main
 * Funções de liberação de memória, visualização da AST e função main
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parser.h"

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
            
        case AST_PRINT_STMT:
            for (int i = 0; i < node->print_stmt.expr_count; i++) {
                free_ast(node->print_stmt.expressions[i]);
            }
            free(node->print_stmt.expressions);
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
            
        case AST_AGGREGATE_TRANSFORM:
            for (int i = 0; i < node->aggregate_transform.agg_arg_count; i++) {
                free_ast(node->aggregate_transform.agg_args[i]);
            }
            free(node->aggregate_transform.agg_args);
            break;
            
        case AST_LITERAL:
            if (node->literal.literal_type == TOKEN_STRING) {
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
            free(node->type_node.type_name);
            free_ast(node->type_node.inner_type);
            for (int i = 0; i < node->type_node.tuple_type_count; i++) {
                free_ast(node->type_node.tuple_types[i]);
            }
            free(node->type_node.tuple_types);
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
        case AST_PRINT_STMT: return "PrintStmt";
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
        
        // Print Statement
        case AST_PRINT_STMT:
            printf("\n");
            if (node->print_stmt.expr_count > 0) {
                for (int i = 0; i < node->print_stmt.expr_count; i++) {
                    print_indent(indent + 1);
                    printf("├─ Expressão %d:\n", i + 1);
                    print_ast(node->print_stmt.expressions[i], indent + 2);
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
            switch (node->literal.literal_type) {
                case TOKEN_INTEGER:
                    printf("INT: %lld", node->literal.int_value);
                    break;
                case TOKEN_FLOAT:
                    printf("FLOAT: %f", node->literal.float_value);
                    break;
                case TOKEN_STRING:
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
            if (node->identifier.id_name != NULL) {
                printf(" '%s'\n", node->identifier.id_name);
            } else {
                printf(" 'NULL_ID'\n");
            }
            break;
            
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

// ==================== SERIALIZAÇÃO JSON PARA VERIFICAÇÃO ====================

static void json_indent(FILE* out, int indent) {
    for (int i = 0; i < indent; i++) {
        fputs("  ", out);
    }
}

static void json_escape_string(FILE* out, const char* s) {
    if (!s) {
        fputs("null", out);
        return;
    }
    fputc('"', out);
    for (const char* p = s; *p; p++) {
        switch (*p) {
            case '\\': fputs("\\\\", out); break;
            case '"': fputs("\\\"", out); break;
            case '\n': fputs("\\n", out); break;
            case '\t': fputs("\\t", out); break;
            case '\r': fputs("\\r", out); break;
            default: fputc(*p, out); break;
        }
    }
    fputc('"', out);
}

static const char* binary_op_to_string(BinaryOp op) {
    switch (op) {
        case BINOP_ADD: return "+";
        case BINOP_SUB: return "-";
        case BINOP_MUL: return "*";
        case BINOP_DIV: return "/";
        case BINOP_MOD: return "%";
        case BINOP_EQ: return "==";
        case BINOP_NEQ: return "!=";
        case BINOP_LT: return "<";
        case BINOP_LTE: return "<=";
        case BINOP_GT: return ">";
        case BINOP_GTE: return ">=";
        case BINOP_AND: return "&&";
        case BINOP_OR: return "||";
        case BINOP_RANGE: return "..";
        default: return "?";
    }
}

static const char* aggregate_to_string(AggregateType agg) {
    switch (agg) {
        case AGG_SUM: return "sum";
        case AGG_MEAN: return "mean";
        case AGG_COUNT: return "count";
        case AGG_MIN: return "min";
        case AGG_MAX: return "max";
        default: return "?";
    }
}

static void serialize_type_node(ASTNode* node, FILE* out, int indent);

static void serialize_ast_node(ASTNode* node, FILE* out, int indent) {
    if (!node) {
        fputs("null", out);
        return;
    }
    json_indent(out, indent);
    fputs("{\n", out);
    json_indent(out, indent + 1);
    fputs("\"type\": \"", out);
    fputs(ast_node_type_name(node->type), out);
    fputs("\",\n", out);

    switch (node->type) {
        case AST_PROGRAM: {
            json_indent(out, indent + 1); fputs("\"declarations\": [\n", out);
            for (int i = 0; i < node->program.decl_count; i++) {
                serialize_ast_node(node->program.declarations[i], out, indent + 2);
                if (i < node->program.decl_count - 1) fputs(",\n", out);
            }
            fputs("\n", out);
            json_indent(out, indent + 1); fputs("]\n", out);
            break;
        }
        case AST_LET_DECL: {
            json_indent(out, indent + 1); fputs("\"name\": ", out); json_escape_string(out, node->let_decl.name); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"typeAnnotation\": ", out); serialize_type_node(node->let_decl.type_annotation, out, indent + 1); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"initializer\": ", out); serialize_ast_node(node->let_decl.initializer, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_FN_DECL: {
            json_indent(out, indent + 1); fputs("\"name\": ", out); json_escape_string(out, node->fn_decl.name); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"params\": [\n", out);
            for (int i = 0; i < node->fn_decl.param_count; i++) {
                serialize_ast_node(node->fn_decl.params[i], out, indent + 2);
                if (i < node->fn_decl.param_count - 1) fputs(",\n", out);
            }
            fputs("\n", out);
            json_indent(out, indent + 1); fputs("],\n", out);
            json_indent(out, indent + 1); fputs("\"returnType\": ", out); serialize_type_node(node->fn_decl.return_type, out, indent + 1); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"body\": ", out); serialize_ast_node(node->fn_decl.body, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_PARAM: {
            json_indent(out, indent + 1); fputs("\"name\": ", out); json_escape_string(out, node->param.param_name); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"paramType\": ", out); serialize_type_node(node->param.param_type, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_DATA_DECL: {
            json_indent(out, indent + 1); fputs("\"name\": ", out); json_escape_string(out, node->data_decl.name); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"fields\": [\n", out);
            for (int i = 0; i < node->data_decl.field_count; i++) {
                serialize_ast_node(node->data_decl.fields[i], out, indent + 2);
                if (i < node->data_decl.field_count - 1) fputs(",\n", out);
            }
            fputs("\n", out);
            json_indent(out, indent + 1); fputs("]\n", out);
            break;
        }
        case AST_FIELD_DECL: {
            json_indent(out, indent + 1); fputs("\"name\": ", out); json_escape_string(out, node->field_decl.field_name); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"fieldType\": ", out); serialize_type_node(node->field_decl.field_type, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_IMPORT_DECL: {
            json_indent(out, indent + 1); fputs("\"module\": ", out); json_escape_string(out, node->import_decl.module_path); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"alias\": ", out); json_escape_string(out, node->import_decl.alias); fputs("\n", out);
            break;
        }
        case AST_EXPORT_DECL: {
            json_indent(out, indent + 1); fputs("\"name\": ", out); json_escape_string(out, node->export_decl.export_name); fputs("\n", out);
            break;
        }
        case AST_IF_STMT: {
            json_indent(out, indent + 1); fputs("\"condition\": ", out); serialize_ast_node(node->if_stmt.condition, out, indent + 1); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"then\": ", out); serialize_ast_node(node->if_stmt.then_block, out, indent + 1); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"else\": ", out); serialize_ast_node(node->if_stmt.else_block, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_FOR_STMT: {
            json_indent(out, indent + 1); fputs("\"iterator\": ", out); json_escape_string(out, node->for_stmt.iterator); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"iterable\": ", out); serialize_ast_node(node->for_stmt.iterable, out, indent + 1); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"body\": ", out); serialize_ast_node(node->for_stmt.body, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_RETURN_STMT: {
            json_indent(out, indent + 1); fputs("\"value\": ", out); serialize_ast_node(node->return_stmt.value, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_PRINT_STMT: {
            json_indent(out, indent + 1); fputs("\"args\": [\n", out);
            for (int i = 0; i < node->print_stmt.expr_count; i++) {
                serialize_ast_node(node->print_stmt.expressions[i], out, indent + 2);
                if (i < node->print_stmt.expr_count - 1) fputs(",\n", out);
            }
            fputs("\n", out);
            json_indent(out, indent + 1); fputs("]\n", out);
            break;
        }
        case AST_EXPR_STMT: {
            json_indent(out, indent + 1); fputs("\"expression\": ", out); serialize_ast_node(node->expr_stmt.expression, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_BLOCK: {
            json_indent(out, indent + 1); fputs("\"statements\": [\n", out);
            for (int i = 0; i < node->block.stmt_count; i++) {
                serialize_ast_node(node->block.statements[i], out, indent + 2);
                if (i < node->block.stmt_count - 1) fputs(",\n", out);
            }
            fputs("\n", out);
            json_indent(out, indent + 1); fputs("]\n", out);
            break;
        }
        case AST_BINARY_EXPR: {
            json_indent(out, indent + 1); fputs("\"op\": ", out); json_escape_string(out, binary_op_to_string(node->binary_expr.op)); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"left\": ", out); serialize_ast_node(node->binary_expr.left, out, indent + 1); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"right\": ", out); serialize_ast_node(node->binary_expr.right, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_UNARY_EXPR: {
            json_indent(out, indent + 1); fputs("\"op\": ", out); json_escape_string(out, node->unary_expr.op == UNOP_NEG ? "-" : "!"); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"operand\": ", out); serialize_ast_node(node->unary_expr.operand, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_CALL_EXPR: {
            json_indent(out, indent + 1); fputs("\"callee\": ", out); serialize_ast_node(node->call_expr.callee, out, indent + 1); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"args\": [\n", out);
            for (int i = 0; i < node->call_expr.arg_count; i++) {
                serialize_ast_node(node->call_expr.arguments[i], out, indent + 2);
                if (i < node->call_expr.arg_count - 1) fputs(",\n", out);
            }
            fputs("\n", out);
            json_indent(out, indent + 1); fputs("]\n", out);
            break;
        }
        case AST_INDEX_EXPR: {
            json_indent(out, indent + 1); fputs("\"object\": ", out); serialize_ast_node(node->index_expr.object, out, indent + 1); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"index\": ", out); serialize_ast_node(node->index_expr.index, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_MEMBER_EXPR: {
            json_indent(out, indent + 1); fputs("\"object\": ", out); serialize_ast_node(node->member_expr.object, out, indent + 1); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"member\": ", out); json_escape_string(out, node->member_expr.member); fputs("\n", out);
            break;
        }
        case AST_ASSIGN_EXPR: {
            json_indent(out, indent + 1); fputs("\"target\": ", out); serialize_ast_node(node->assign_expr.target, out, indent + 1); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"value\": ", out); serialize_ast_node(node->assign_expr.value, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_LAMBDA_EXPR: {
            json_indent(out, indent + 1); fputs("\"params\": [\n", out);
            for (int i = 0; i < node->lambda_expr.lambda_param_count; i++) {
                serialize_ast_node(node->lambda_expr.lambda_params[i], out, indent + 2);
                if (i < node->lambda_expr.lambda_param_count - 1) fputs(",\n", out);
            }
            fputs("\n", out);
            json_indent(out, indent + 1); fputs("],\n", out);
            json_indent(out, indent + 1); fputs("\"body\": ", out); serialize_ast_node(node->lambda_expr.lambda_body, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_PIPELINE_EXPR: {
            json_indent(out, indent + 1); fputs("\"stages\": [\n", out);
            for (int i = 0; i < node->pipeline_expr.stage_count; i++) {
                serialize_ast_node(node->pipeline_expr.stages[i], out, indent + 2);
                if (i < node->pipeline_expr.stage_count - 1) fputs(",\n", out);
            }
            fputs("\n", out);
            json_indent(out, indent + 1); fputs("]\n", out);
            break;
        }
        case AST_FILTER_TRANSFORM: {
            json_indent(out, indent + 1); fputs("\"predicate\": ", out); serialize_ast_node(node->filter_transform.filter_predicate, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_MAP_TRANSFORM: {
            json_indent(out, indent + 1); fputs("\"mapper\": ", out); serialize_ast_node(node->map_transform.map_function, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_REDUCE_TRANSFORM: {
            json_indent(out, indent + 1); fputs("\"initial\": ", out); serialize_ast_node(node->reduce_transform.initial_value, out, indent + 1); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"reducer\": ", out); serialize_ast_node(node->reduce_transform.reducer, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_SELECT_TRANSFORM: {
            json_indent(out, indent + 1); fputs("\"columns\": [", out);
            for (int i = 0; i < node->select_transform.column_count; i++) {
                json_escape_string(out, node->select_transform.columns[i]);
                if (i < node->select_transform.column_count - 1) fputs(", ", out);
            }
            fputs("]\n", out);
            break;
        }
        case AST_GROUPBY_TRANSFORM: {
            json_indent(out, indent + 1); fputs("\"groupColumns\": [", out);
            for (int i = 0; i < node->groupby_transform.group_column_count; i++) {
                json_escape_string(out, node->groupby_transform.group_columns[i]);
                if (i < node->groupby_transform.group_column_count - 1) fputs(", ", out);
            }
            fputs("]\n", out);
            break;
        }
        case AST_AGGREGATE_TRANSFORM: {
            json_indent(out, indent + 1); fputs("\"agg\": ", out); json_escape_string(out, aggregate_to_string(node->aggregate_transform.agg_type)); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"args\": [\n", out);
            for (int i = 0; i < node->aggregate_transform.agg_arg_count; i++) {
                serialize_ast_node(node->aggregate_transform.agg_args[i], out, indent + 2);
                if (i < node->aggregate_transform.agg_arg_count - 1) fputs(",\n", out);
            }
            fputs("\n", out);
            json_indent(out, indent + 1); fputs("]\n", out);
            break;
        }
        case AST_LITERAL: {
            json_indent(out, indent + 1); fputs("\"literalType\": ", out);
            switch (node->literal.literal_type) {
                case TOKEN_INTEGER:
                    fputs("\"Int\",\n", out);
                    json_indent(out, indent + 1); fputs("\"value\": ", out); fprintf(out, "%lld\n", node->literal.int_value);
                    break;
                case TOKEN_FLOAT:
                    fputs("\"Float\",\n", out);
                    json_indent(out, indent + 1); fputs("\"value\": ", out); fprintf(out, "%f\n", node->literal.float_value);
                    break;
                case TOKEN_STRING:
                    fputs("\"String\",\n", out);
                    break;
                case TOKEN_BOOL_TYPE:
                    fputs("\"Bool\",\n", out);
                    break;
                default:
                    fputs("\"Unknown\"\n", out);
                    break;
            }
            if (node->literal.literal_type == TOKEN_STRING) {
                json_indent(out, indent + 1); fputs("\"value\": ", out); json_escape_string(out, node->literal.string_value); fputs("\n", out);
            } else if (node->literal.literal_type == TOKEN_BOOL_TYPE) {
                json_indent(out, indent + 1); fputs("\"value\": ", out); fputs(node->literal.bool_value ? "true\n" : "false\n", out);
            }
            break;
        }
        case AST_IDENTIFIER: {
            json_indent(out, indent + 1); fputs("\"name\": ", out); json_escape_string(out, node->identifier.id_name); fputs("\n", out);
            break;
        }
        case AST_ARRAY_LITERAL: {
            json_indent(out, indent + 1); fputs("\"elements\": [\n", out);
            for (int i = 0; i < node->array_literal.element_count; i++) {
                serialize_ast_node(node->array_literal.elements[i], out, indent + 2);
                if (i < node->array_literal.element_count - 1) fputs(",\n", out);
            }
            fputs("\n", out);
            json_indent(out, indent + 1); fputs("]\n", out);
            break;
        }
        case AST_LOAD_EXPR: {
            json_indent(out, indent + 1); fputs("\"path\": ", out); json_escape_string(out, node->load_expr.file_path); fputs("\n", out);
            break;
        }
        case AST_SAVE_EXPR: {
            json_indent(out, indent + 1); fputs("\"data\": ", out); serialize_ast_node(node->save_expr.data, out, indent + 1); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"path\": ", out); json_escape_string(out, node->save_expr.save_path); fputs("\n", out);
            break;
        }
        case AST_RANGE_EXPR: {
            json_indent(out, indent + 1); fputs("\"start\": ", out); serialize_ast_node(node->range_expr.range_start, out, indent + 1); fputs(",\n", out);
            json_indent(out, indent + 1); fputs("\"end\": ", out); serialize_ast_node(node->range_expr.range_end, out, indent + 1); fputs("\n", out);
            break;
        }
        case AST_TYPE:
            json_indent(out, indent + 1); fputs("\"typeNode\": ", out); serialize_type_node(node, out, indent + 1); fputs("\n", out);
            break;
        default:
            json_indent(out, indent + 1); fputs("\"detail\": null\n", out);
            break;
    }
    json_indent(out, indent);
    fputs("}", out);
}

static void serialize_type_node(ASTNode* node, FILE* out, int indent) {
    if (!node) {
        fputs("null", out);
        return;
    }
    json_indent(out, indent);
    fputs("{\"kind\": ", out);
    const char* kind = node->type_node.type_name ? node->type_node.type_name : "?";
    json_escape_string(out, kind);
    if (node->type_node.type_kind == TOKEN_LBRACKET) {
        fputs(", \"arrayOf\": ", out);
        serialize_type_node(node->type_node.inner_type, out, 0);
    } else if (node->type_node.type_kind == TOKEN_LPAREN) {
        fputs(", \"tuple\": [", out);
        for (int i = 0; i < node->type_node.tuple_type_count; i++) {
            serialize_type_node(node->type_node.tuple_types[i], out, 0);
            if (i < node->type_node.tuple_type_count - 1) fputs(", ", out);
        }
        fputs("]", out);
    }
    fputs("}", out);
}

bool write_ast_json(ASTNode* node, const char* filepath) {
    FILE* out = fopen(filepath, "w");
    if (!out) return false;
    serialize_ast_node(node, out, 0);
    fputc('\n', out);
    fclose(out);
    return true;
}

void test_parser_with_code(const char* code) {
    printf("\n════════════════════════════════════════════════════════════\n");
    printf("Código DataLang:\n");
    printf("════════════════════════════════════════════════════════════\n");
    printf("%s\n", code);
    printf("════════════════════════════════════════════════════════════\n");
    
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
    
    Parser* parser = create_parser(tokens);
    if (!parser) {
        printf("Erro ao criar parser\n");
        free_token_stream(tokens);
        free_afd(afd);
        return;
    }
    
    ASTNode* ast = parse(parser);
    
    if (ast && !parser->had_error) {
        printf("\n╔════════════════════════════════════════════════════════════╗\n");
        printf("║                   ÁRVORE SINTÁTICA ABSTRATA                ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n\n");
        print_ast(ast, 0);
        printf("\nAST construída com sucesso\n");
    }
    
    if (ast) free_ast(ast);
    free_parser(parser);
    free_token_stream(tokens);
    free_afd(afd);
}
